#ifndef _OP_PACKET_GENERATOR_LEARNING_HPP_
#define _OP_PACKET_GENERATOR_LEARNING_HPP_

#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <functional>

#include "packetio/generic_interface.hpp"

#include "lib/packet/type/ipv4_address.hpp"
#include "lib/packet/type/ipv6_address.hpp"
#include "lib/packet/type/mac_address.hpp"

#include "core/op_core.h"

namespace openperf::packet::generator {

class learning_state_machine;

using resolve_complete_callback =
    std::function<void(const learning_state_machine&)>;

using learning_result_map_ipv4 =
    std::unordered_map<libpacket::type::ipv4_address,
                       std::optional<libpacket::type::mac_address>>;

struct ipv6_nd_result
{
    std::optional<libpacket::type::ipv6_address> next_hop_address;
    std::optional<libpacket::type::mac_address> next_hop_mac;
    int neighbor_cache_offset;
};

using learning_result_map_ipv6 =
    std::unordered_map<libpacket::type::ipv6_address, ipv6_nd_result>;

struct learning_results
{
    learning_result_map_ipv4 ipv4;
    learning_result_map_ipv6 ipv6;
};

// Concrete types representing individual states.
struct state_start
{};
struct state_learning
{};
struct state_done
{};
struct state_timeout
{};

using learning_state = std::variant<std::monostate,
                                    state_start,
                                    state_learning,
                                    state_done,
                                    state_timeout>;

enum class learning_resolved_state {
    unsupported = 0, // This is used at higher levels.
    unresolved,
    resolving,
    resolved,
    timed_out
};

std::string to_string(const learning_resolved_state& state);

/**
 * @brief Finite State Machine class that handles MAC learning.
 *
 */
class learning_state_machine
{
public:
    explicit learning_state_machine(
        core::event_loop& loop, packetio::interface::generic_interface& intf)
        : m_loop(loop)
        , m_loop_timeout_id(0)
        , m_polls_remaining(0)
        , m_interface(intf)
    {}

    ~learning_state_machine()
    {
        // We might be in the process of learning when deleted.
        // Don't leave a stale timer handle in the shared event loop.
        stop_learning();
    }

    /**
     * @brief Start MAC learning process.
     *
     * @param interface lwip interface to use for learning
     * @param to_learn_ipv4 list of IPv4 addresses to learn
     * @param to_learn_ipv6 list of IPv6 addresses to learn
     * @return true if learning started successfully
     * @return false learning did not start
     */
    bool start_learning(
        const std::unordered_set<libpacket::type::ipv4_address>& to_learn_ipv4,
        const std::unordered_set<libpacket::type::ipv6_address>& to_learn_ipv6,
        resolve_complete_callback callback);

    /**
     * @brief Retry failed MAC learning items.
     *
     * @return true if learning started successfully
     * @return false learning did not start
     */
    bool retry_learning();

    /**
     * @brief Update resolved addresses from lwip stack MAC learning cache.
     *
     * @return 0 if additional checks are required, -1 otherwise.
     *
     * Return values are dictated by core::op_event_loop().
     */
    int check_learning();

    /**
     * @brief Drive state machine to completion state (state_done if all entries
     * resolved, state_timeout otherwise.)
     *
     */
    void stop_learning();

    /**
     * @brief Get reference to ARP results.
     *
     * @return const reference to results data structure.
     */
    const learning_results& results() const { return m_results; }

    /**
     * @brief Did learning resolve all requested MAC addresses?
     *
     * Will return false if called before learning started.
     *
     * @return true all requested MAC addresses resolved
     * @return false at least one requested MAC address did not resolve.
     */
    bool resolved() const
    {
        return (std::holds_alternative<state_done>(m_current_state));
    }

    /**
     * @brief Shorthand for state machine starting up or in process of learning.
     *
     * @return true state machine learning in progress.
     * @return false state machine learning stopped.
     */
    bool in_progress() const
    {
        return (std::holds_alternative<state_start>(m_current_state)
                || std::holds_alternative<state_learning>(m_current_state));
    }

    learning_resolved_state state() const;

    /**
     * @brief Get instance's event loop timeout ID.
     *
     * @return uint32_t zero if no timeout scheduled, non-zero if timeout
     * scheduled.
     */
    uint32_t timeout_id() const { return m_loop_timeout_id; }

    void set_timeout_id(uint32_t id) { m_loop_timeout_id = id; }

private:
    /**
     * @brief Internal impl method used by start/retry methods since
     * implementation is mostly the same.
     *
     * @return true learning started
     * @return false learning did nto start
     */
    bool start_learning_impl();

    /**
     * @brief Internal impl method to start polling for ARP request resolved
     * status.
     *
     * @return true polling started successfully.
     * @return false polling did not start successfully.
     */
    bool start_status_polling();

    std::reference_wrapper<core::event_loop> m_loop;
    uint32_t m_loop_timeout_id;
    int m_polls_remaining;

    packetio::interface::generic_interface m_interface;
    learning_results m_results;
    learning_state m_current_state;

    resolve_complete_callback m_resolved_callback;
};

} // namespace openperf::packet::generator

#endif /* _OP_PACKET_GENERATOR_LEARNING_HPP_ */