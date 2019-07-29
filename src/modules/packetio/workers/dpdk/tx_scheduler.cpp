#include <sys/timerfd.h>
#include <unistd.h>

#include "packetio/workers/dpdk/tx_scheduler.h"

namespace icp::packetio::dpdk {

namespace schedule {

constexpr bool operator>(const entry& left, const entry& right)
{
    return (left.deadline > right.deadline);
}

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

std::string_view to_string(state& state)
{
    return (std::visit(overloaded_visitor(
                           [](const state_idle&) {
                               return ("idle");
                           },
                           [](const state_link_check&) {
                               return ("link_check");
                           },
                           [](const state_running&) {
                               return ("running");
                           }),
                       state));
}

}

/*
 * Callback function for DPDK use. It just clears the file descriptor for
 * us after the timer fd fires.
 */
static void read_timer(int fd, void* arg)
{
    auto scheduler = reinterpret_cast<tx_scheduler*>(arg);
    uint64_t counter = 0;
    auto error = read(fd, &counter, sizeof(counter));
    if (error == -1 && errno == EAGAIN) {
        ICP_LOG(ICP_LOG_WARNING, "Spurious tx scheduler wakeup for %u:%u\n",
                scheduler->port_id(), scheduler->queue_id());
    }
}

static int set_timer_interval(int fd, std::chrono::nanoseconds ns)
{
    using namespace std::literals::chrono_literals;
    using ts_s  = std::chrono::duration<decltype(std::declval<timespec>().tv_sec)>;
    using ts_ns = std::chrono::duration<decltype(std::declval<timespec>().tv_nsec),
                                        std::nano>;

    auto timer_value = itimerspec{
        .it_interval = { std::chrono::duration_cast<ts_s>(ns).count(),
                         std::chrono::duration_cast<ts_ns>(ns % 1s).count() },
        .it_value    = { 0, 1 }  /* fire immediately */
    };

    return (timerfd_settime(fd, 0, &timer_value, nullptr));
}

static int set_timer_oneshot(int fd, std::chrono::nanoseconds ns)
{
    using namespace std::literals::chrono_literals;
    using ts_s  = std::chrono::duration<decltype(std::declval<timespec>().tv_sec)>;
    using ts_ns = std::chrono::duration<decltype(std::declval<timespec>().tv_nsec),
                                        std::nano>;

    auto timer_value = itimerspec{
        .it_interval = { 0, 0 },
        .it_value    = { std::chrono::duration_cast<ts_s>(ns).count(),
                         std::chrono::duration_cast<ts_ns>(ns % 1s).count() }
    };

    return (timerfd_settime(fd, 0, &timer_value, nullptr));
}

std::chrono::nanoseconds next_deadline(const packets::generic_source& source)
{
    using ns = std::chrono::nanoseconds;
    return (units::get_period<ns>(source.packet_rate() / source.burst_size()));
}

tx_scheduler::tx_scheduler(const worker::tib& tib, uint16_t port_idx, uint16_t queue_idx)
    : m_tib(tib)
    , m_portid(port_idx)
    , m_queueid(queue_idx)
    , m_timerfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK))
{
    if (m_timerfd == -1) {
        throw std::runtime_error("Could not create kernel timer: "
                                 + std::string(strerror(errno)));
    }

    using namespace std::literals::chrono_literals;
    if (set_timer_interval(m_timerfd, 1s) == -1) {
        throw std::runtime_error("Could not set kernel timer: "
                                 + std::string(strerror(errno)));
    }
}

tx_scheduler::~tx_scheduler()
{
    close(m_timerfd);
}

/*
 * Luckily, std::priority_queue objects store their underlying container
 * as a protected member, c.  We use this fact and some template hackery
 * to get access to the underlying container.  We need this in order to
 * determine what it contains and what it doesn't when we perform a
 * rescheduling operation.
 *
 * A good explanation of how this works can be found here:
 * https://stackoverflow.com/questions/29289974/what-does-the-operator-do/29291621#29291621
 */
template <class Container>
const typename Container::container_type& get_container(Container& container)
{
    struct accessor : Container {
        static const typename Container::container_type& get (Container& container)
        {
            return container.*&accessor::c;
        }
    };

    return (accessor::get(container));
}

/*
 * Look for new sources that we don't have scheduled and add them to
 * the schedule.
 */
void tx_scheduler::do_reschedule(const schedule::time_point& now)
{
    const auto& priority_vec = get_container(m_schedule);

    for (const auto& key_source : m_tib.get_sources(port_id(), queue_id())) {
        const auto& key = key_source.first;
        const auto& source = key_source.second;
        auto found = std::find_if(std::begin(priority_vec), std::end(priority_vec),
                                  [&](const auto& entry) {
                                      return (key == entry.key);
                                  });
        if (found == std::end(priority_vec) && source.active()) {
            m_schedule.push({ now + next_deadline(source), key });
        }
    }

    using namespace std::literals::chrono_literals;
    m_time_reschedule += 1s;  /* Update from previous value to prevent drift */
}

uint16_t tx_scheduler::port_id() const
{
    return (m_portid);
}

uint16_t tx_scheduler::queue_id() const
{
    return (m_queueid);
}

int tx_scheduler::event_fd() const
{
    return (m_timerfd);
}

void* tx_scheduler::event_callback_argument()
{
    return (this);
}

pollable_event<tx_scheduler>::event_callback tx_scheduler::event_callback_function() const
{
    return (read_timer);
}

static bool link_down(uint16_t port_idx)
{
    auto link = rte_eth_link{};
    rte_eth_link_get_nowait(port_idx, &link);
    return (link.link_status == ETH_LINK_DOWN);
}

static bool have_sources(const worker::tib& tib, uint16_t port_idx, uint16_t queue_idx)
{
    /* Check the forwarding table for matching events */
    auto range = tib.get_sources(port_idx, queue_idx);
    return (std::distance(range.first, range.second));
}

std::optional<schedule::state> tx_scheduler::on_timeout(const schedule::state_idle&)
{
    /* No sources --> nothing to do */
    if (!have_sources(m_tib, port_id(), queue_id())) return (std::nullopt);

    /* Next state depends on link */
    if (link_down(port_id())) {
        return (schedule::state_link_check{});
    } else {
        return (schedule::state_running{});
    }
}

std::optional<schedule::state> tx_scheduler::on_timeout(const schedule::state_link_check&)
{
    if (link_down(port_id())) return (std::nullopt);

    /* Link is up; Next state depends on whether we have events to schedule */
    if (have_sources(m_tib, port_id(), queue_id())) {
        return (schedule::state_running{});
    } else {
        return (schedule::state_idle{});
    }
}

std::optional<schedule::state> tx_scheduler::on_timeout(const schedule::state_running&)
{
    assert(!m_schedule.empty());

    schedule::time_point now;
    std::array<rte_mbuf*, max_tx_burst_size> outgoing;

    if (link_down(port_id())) return (schedule::state_link_check{});

    while (m_schedule.top().deadline < (now = schedule::clock::now())) {
        /* Grab the event from the schedule */
        auto& [deadline, key] = m_schedule.top();

        auto source = m_tib.get_source(key);
        if (!source) {
            /* Source could have been removed while we were idle */
            m_schedule.pop();
            continue;
        }

        /*
         * Run tx event; obviously need a better implementation to handle packets we
         * can't send immediately.
         */
        auto to_send = source->pull(reinterpret_cast<void**>(outgoing.data()),
                                    std::min(static_cast<uint16_t>(outgoing.size()),
                                             source->burst_size()));
        auto sent = rte_eth_tx_burst(port_id(), queue_id(), outgoing.data(), to_send);

        ICP_LOG(ICP_LOG_TRACE, "Transmitted %u of %u packets on %u:%u from source %s\n",
                sent, to_send, port_id(), queue_id(), source->id().c_str());

        /* Re-add entry to schedule if it still active */
        if (source->active()) {
            /*
             * Always update from the previous deadline value, as we don't want
             * our intervals to drift.
             */
            m_schedule.push({deadline + next_deadline(*source), key});
        }

        m_schedule.pop();
    }

    if (m_schedule.empty()) return (on_timeout(schedule::state_idle{}));

    if (now > m_time_reschedule) do_reschedule(now);

    /* Update timer for next event */
    set_timer_oneshot(m_timerfd,
                      std::min(m_schedule.top().deadline, m_time_reschedule) - now);
    return (std::nullopt);
}

void tx_scheduler::on_transition(const schedule::state_idle&)
{
    assert(m_schedule.empty());

    using namespace std::literals::chrono_literals;
    set_timer_interval(m_timerfd, 1s);
}

void tx_scheduler::on_transition(const schedule::state_link_check&)
{
    /* Drop any scheduled items as we can't do anything with them without a link */
    while (!m_schedule.empty()) {
        m_schedule.pop();
    }

    using namespace std::literals::chrono_literals;
    set_timer_interval(m_timerfd, 500ms);
}

void tx_scheduler::on_transition(const schedule::state_running&)
{
    assert(m_schedule.empty());

    auto now = schedule::clock::now();

    /* Generate a schedule for all available entities */
    for (auto& [key, source] : m_tib.get_sources(port_id(), queue_id())) {
        m_schedule.push({now + next_deadline(source), key});
    }

    /* Adjust timer to match schedule */
    using namespace std::literals::chrono_literals;
    m_time_reschedule = now + 1s;
    set_timer_oneshot(m_timerfd,
                      std::min(m_schedule.top().deadline, m_time_reschedule) - now);
}

}
