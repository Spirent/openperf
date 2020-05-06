#ifndef _OP_PACKETIO_DPDK_TX_SCHEDULER_HPP_
#define _OP_PACKETIO_DPDK_TX_SCHEDULER_HPP_

#include <chrono>
#include <queue>
#include <optional>
#include <variant>

#include "packetio/generic_source.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"
#include "packetio/workers/dpdk/pollable_event.tcc"

namespace openperf::packetio::dpdk {

namespace schedule {

using clock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<clock>;

struct entry
{
    time_point deadline;
    worker::tib::key_type key;
};

constexpr bool operator>(const entry& left, const entry& right);

struct state_idle
{}; /* No events are scheduled */
struct state_link_check
{}; /* Events are scheduled; link is down */
struct state_running
{ /* Events are schedule; link is up */
    mutable bool reschedule = false;
};
struct state_blocked /* Events are scheduled; link is up; queue is full */
{
    mutable uint16_t remaining;
    entry entry;
};

using state =
    std::variant<state_idle, state_link_check, state_running, state_blocked>;

std::string_view to_string(state&);

template <typename Derived, typename StateVariant> class finite_state_machine
{
    StateVariant m_state;

public:
    void run()
    {
        auto& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](auto& state) -> std::optional<StateVariant> {
                return (child.on_timeout(state));
            },
            m_state);

        if (next_state) {
            OP_LOG(OP_LOG_TRACE,
                   "Tx port scheduler %u:%u: %.*s --> %.*s\n",
                   child.port_id(),
                   child.queue_id(),
                   static_cast<int>(to_string(m_state).length()),
                   to_string(m_state).data(),
                   static_cast<int>(to_string(*next_state).length()),
                   to_string(*next_state).data());

            m_state = *next_state;
            std::visit([&](auto& state) { child.on_transition(state); },
                       m_state);
        }
    }
};

} // namespace schedule

class tx_scheduler
    : public pollable_event<tx_scheduler>
    , public schedule::finite_state_machine<tx_scheduler, schedule::state>
{
    const worker::tib& m_tib;
    uint16_t m_portid;
    uint16_t m_queueid;
    int m_timerfd;

    std::vector<rte_mbuf*> m_buffer;

    /* By default, priority_queue sorts by std::less<> which causes the
     * largest element to appear as top().  For our use, we want the
     * smallest value, as that represents the next deadline, so we use
     * std::greater<> instead.
     */
    std::priority_queue<schedule::entry,
                        std::vector<schedule::entry>,
                        std::greater<>>
        m_schedule;

    schedule::time_point m_time_reschedule;

    void do_reschedule(const schedule::time_point& now);

public:
    tx_scheduler(const worker::tib& tib, uint16_t port_idx, uint16_t queue_idx);
    ~tx_scheduler();

    uint16_t port_id() const;
    uint16_t queue_id() const;

    int event_fd() const;
    void* event_callback_argument();
    pollable_event<tx_scheduler>::event_callback
    event_callback_function() const;

    /*
     * Because we expect state transitions to be the rare, e.g. timeouts
     * normally don't change the current state, we divide the event handling
     * process into two sets of functions: on_timeout and on_transition.
     *
     * The on_timeout functions contain the action required for a timeout
     * in the specified state and the decision making logic needed on
     * whether to transition to a new state or not.
     *
     * The on_transition functions handle updating the timer and other
     * internal variables needed to run in the specified state.
     */
    std::optional<schedule::state> on_timeout(const schedule::state_idle&);
    std::optional<schedule::state>
    on_timeout(const schedule::state_link_check&);
    std::optional<schedule::state> on_timeout(const schedule::state_running&);
    std::optional<schedule::state> on_timeout(const schedule::state_blocked&);

    void on_transition(const schedule::state_idle&);
    void on_transition(const schedule::state_link_check&);
    void on_transition(const schedule::state_running&);
    void on_transition(const schedule::state_blocked&);
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_TX_SCHEDULER_HPP_ */
