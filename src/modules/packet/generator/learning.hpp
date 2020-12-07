#ifndef _OP_PACKET_GENERATOR_LEARNING_HPP_
#define _OP_PACKET_GENERATOR_LEARNING_HPP_

#include <vector>
#include <map>
#include <variant>
#include <functional>

#include "lwip/netif.h"

#include "lib/packet/type/ipv4_address.hpp"
#include "lib/packet/type/mac_address.hpp"

#include "core/op_core.h"

namespace openperf::packet::generator {

class learning_state_machine;

using resolve_complete_callback =
    std::function<void(const learning_state_machine&)>;

using unresolved = std::monostate;
using mac_type = std::variant<unresolved, libpacket::type::mac_address>;

using learning_result_map =
    std::unordered_map<libpacket::type::ipv4_address, mac_type>;

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

/**
 * @brief Finite State Machine class that handles MAC learning.
 *
 */
class learning_state_machine
{
public:
    explicit learning_state_machine(core::event_loop& loop, netif* intf)
        : m_loop(loop)
        , m_loop_timeout_id(0)
        , m_polls_remaining(0)
        , m_intf(intf)
    {
        if (m_intf == nullptr) {
            throw std::invalid_argument(
                "Must pass non-null interface pointer to learning.");
        }
    }

    /**
     * @brief Start MAC learning process.
     *
     * @param interface lwip interface to use for learning
     * @param to_learn list of IP addresses to learn
     * @return true if learning started successfully
     * @return false learning did not start
     */
    bool
    start_learning(const std::vector<libpacket::type::ipv4_address>& to_learn,
                   resolve_complete_callback callback);

    /**
     * @brief Retry failed MAC learning items.
     *
     * @return true if learning started successfully
     * @return false learning did not start
     */
    bool retry_failed();

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
    const learning_result_map& results() const { return m_results; }

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

    core::event_loop& m_loop;
    uint32_t m_loop_timeout_id;
    int m_polls_remaining;

    netif* m_intf;
    learning_result_map m_results;
    learning_state m_current_state;

    resolve_complete_callback m_resolved_callback;
};

} // namespace openperf::packet::generator

#endif /* _OP_PACKET_GENERATOR_LEARNING_HPP_ */