#include <sys/timerfd.h>
#include <unistd.h>

#include "packetio/drivers/dpdk/arg_parser.hpp"
#include "packetio/workers/dpdk/tx_scheduler.hpp"
#include "timesync/chrono.hpp"
#include "units/data-rates.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packetio::dpdk {

using namespace std::literals::chrono_literals;
static constexpr auto block_poll = 100ns;
static constexpr auto idle_poll = 100ms;
static constexpr auto link_poll = 100us;
static constexpr auto min_poll = 1ns;
static constexpr auto schedule_poll = idle_poll;

static constexpr auto quanta_bits = 512; /* 1 quanta is 512 bit-times */

namespace schedule {

constexpr bool operator>(const entry& left, const entry& right)
{
    return (left.deadline > right.deadline);
}

std::string_view to_string(state& state)
{
    return (
        std::visit(utils::overloaded_visitor(
                       [](const state_idle&) { return ("idle"); },
                       [](const state_link_check&) { return ("link_check"); },
                       [](const state_running&) { return ("running"); },
                       [](const state_blocked&) { return ("blocked"); }),
                   state));
}

} // namespace schedule

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
        OP_LOG(OP_LOG_WARNING,
               "Spurious tx scheduler wakeup for %u:%u\n",
               scheduler->port_id(),
               scheduler->queue_id());
    }
}

static int set_timer_interval(int fd, std::chrono::nanoseconds ns)
{
    using namespace std::literals::chrono_literals;
    using ts_s =
        std::chrono::duration<decltype(std::declval<timespec>().tv_sec)>;
    using ts_ns =
        std::chrono::duration<decltype(std::declval<timespec>().tv_nsec),
                              std::nano>;

    auto timer_value = itimerspec{
        .it_interval = {std::chrono::duration_cast<ts_s>(ns).count(),
                        std::chrono::duration_cast<ts_ns>(ns % 1s).count()},
        .it_value = {0, 1} /* fire immediately */
    };

    return (timerfd_settime(fd, 0, &timer_value, nullptr));
}

static int set_timer_oneshot(int fd, std::chrono::nanoseconds ns)
{
    assert(ns.count() > 0);

    using namespace std::literals::chrono_literals;
    using ts_s =
        std::chrono::duration<decltype(std::declval<timespec>().tv_sec)>;
    using ts_ns =
        std::chrono::duration<decltype(std::declval<timespec>().tv_nsec),
                              std::nano>;

    auto timer_value = itimerspec{
        .it_interval = {0, 0},
        .it_value = {std::chrono::duration_cast<ts_s>(ns).count(),
                     std::chrono::duration_cast<ts_ns>(ns % 1s).count()}};

    return (timerfd_settime(fd, 0, &timer_value, nullptr));
}

template <typename Source>
std::chrono::nanoseconds next_deadline(const Source& source)
{
    using ns = std::chrono::nanoseconds;
    return (units::to_duration<ns>(source.packet_rate() / source.burst_size()));
}

tx_scheduler::tx_scheduler(const worker::tib& tib,
                           uint16_t port_idx,
                           uint16_t queue_idx)
    : m_tib(tib)
    , m_portid(port_idx)
    , m_queueid(queue_idx)
    , m_timerfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK))
{
    if (m_timerfd == -1) {
        throw std::runtime_error("Could not create kernel timer: "
                                 + std::string(strerror(errno)));
    }

    if (set_timer_interval(m_timerfd, idle_poll) == -1) {
        throw std::runtime_error("Could not set kernel timer: "
                                 + std::string(strerror(errno)));
    }
}

tx_scheduler::~tx_scheduler() { close(m_timerfd); }

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
    struct accessor : Container
    {
        static const typename Container::container_type&
        get(Container& container)
        {
            return container.*&accessor::c;
        }
    };

    return (accessor::get(container));
}

/*
 * Look for new, active sources that we don't have scheduled and add them to
 * the schedule.
 */
void tx_scheduler::do_reschedule(const schedule::time_point& now)
{
    const auto& priority_vec = get_container(m_schedule);

    for (const auto& key_source : m_tib.get_sources(port_id(), queue_id())) {
        const auto key = worker::tib::to_safe_key(key_source->first);
        const auto& source = key_source->second;
        if (source.active()
            && std::none_of(
                std::begin(priority_vec),
                std::end(priority_vec),
                [&](const auto& entry) { return (key == entry.key); })) {
            m_schedule.push({now + next_deadline(source), key});
        }
    }

    /*
     * Updating from the previous rescheduling time prevents drift.
     * However, if we've been buffering packets, we could be running behind, so
     * make sure our next reschedule time is in the future.
     */
    while (m_time_reschedule < now) { m_time_reschedule += schedule_poll; }
}

uint16_t tx_scheduler::port_id() const { return (m_portid); }

uint16_t tx_scheduler::queue_id() const { return (m_queueid); }

int tx_scheduler::event_fd() const { return (m_timerfd); }

void* tx_scheduler::event_callback_argument() { return (this); }

pollable_event<tx_scheduler>::event_callback
tx_scheduler::event_callback_function() const
{
    return (read_timer);
}

static bool link_down(uint16_t port_idx)
{
    auto link = rte_eth_link{};
    rte_eth_link_get_nowait(port_idx, &link);
    return (link.link_status == ETH_LINK_DOWN);
}

static bool have_active_sources(const worker::tib& tib,
                                uint16_t port_idx,
                                uint16_t queue_idx)
{
    /* Check the forwarding table for matching events */
    auto range = tib.get_sources(port_idx, queue_idx);
    return (std::any_of(range.first, range.second, [](const auto& key_source) {
        return (key_source->second.active());
    }));
}

std::optional<schedule::state>
tx_scheduler::on_timeout(const schedule::state_idle&)
{
    /* No sources --> nothing to do */
    if (!have_active_sources(m_tib, port_id(), queue_id()))
        return (std::nullopt);

    /* Next state depends on link */
    if (link_down(port_id())) {
        return (schedule::state_link_check{});
    } else {
        return (schedule::state_running{});
    }
}

std::optional<schedule::state>
tx_scheduler::on_timeout(const schedule::state_link_check&)
{
    if (link_down(port_id())) return (std::nullopt);

    /* Link is up; Next state depends on whether we have events to schedule */
    if (have_active_sources(m_tib, port_id(), queue_id())) {
        return (schedule::state_running{});
    } else {
        return (schedule::state_idle{});
    }
}

template <typename T>
static __attribute__((const)) T distribute(T total, T buckets, T n)
{
    assert(n < buckets);
    auto base = total / buckets;
    return (n < total % buckets ? base + 1 : base);
}

static uint32_t get_link_speed_safe(uint16_t port_id)
{
    /* Query the port's link speed */
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);

    /* Caller should only call this when the link is up */
    assert(link.link_status == ETH_LINK_UP);
    return (link.link_speed);
}

static void do_packet_wait(uint16_t port_idx, uint16_t nb_packets)
{
    /*
     * Wait one ethernet quanta for each packet. Since a bit-time
     * takes less that 1 nanosecond at 100G, we need to use
     * picoseconds for our calculations.
     */
    using clock = openperf::timesync::chrono::monotime;
    using mbps = units::rate<uint64_t, units::megabits>;
    using picoseconds = std::chrono::duration<int64_t, std::pico>;

    auto delay =
        units::to_duration<picoseconds>(mbps(get_link_speed_safe(port_idx)))
        * quanta_bits * nb_packets;

    auto until = clock::now() + delay;
    do {
        rte_pause();
        rte_pause();
        rte_pause();
        rte_pause();
    } while (clock::now() <= until);
}

template <typename Source>
static uint16_t do_transmit(uint16_t port_idx,
                            uint16_t queue_idx,
                            Source& source,
                            uint16_t burst_size,
                            std::vector<rte_mbuf*>& untransmitted)
{
    std::array<rte_mbuf*, worker::pkt_burst_size> outgoing;

    /*
     * Divide our source's packet burst into a set of similarly sized chunks
     * that are less than or equal to our packet burst size.
     */
    uint16_t total_sent = 0;
    uint16_t total_to_send = 0;
    uint16_t nb_bursts =
        (burst_size + (worker::pkt_burst_size - 1)) / worker::pkt_burst_size;
    for (uint16_t i = 0; i < nb_bursts; i++) {
        auto loop_burst = distribute(burst_size, nb_bursts, i);
        assert(loop_burst <= outgoing.size());

        /* Grab a burst of packets and attempt to transmit them. */
        auto to_send = source->pull(outgoing.data(), loop_burst);
        total_to_send += to_send;

        auto nb_prep =
            rte_eth_tx_prepare(port_idx, queue_idx, outgoing.data(), to_send);
        if (nb_prep != to_send) {
            OP_LOG(OP_LOG_DEBUG,
                   "Failed preparing packets on %u:%u.  Prepared %u of %u "
                   "packets.  %s.\n",
                   port_idx,
                   queue_idx,
                   nb_prep,
                   to_send,
                   rte_strerror(rte_errno));
            std::copy(outgoing.begin(),
                      outgoing.begin() + to_send,
                      std::back_inserter(untransmitted));
            break;
        }

        auto sent =
            rte_eth_tx_burst(port_idx, queue_idx, outgoing.data(), to_send);

        /*
         * If we were unable to send all of the packets in our burst, take
         * a break and try again.
         */
        if (sent < to_send) {
            do_packet_wait(port_idx, to_send - sent);
            sent += rte_eth_tx_burst(
                port_idx, queue_idx, outgoing.data() + sent, to_send - sent);

            /* Queue is still full after retrying; buffer packets */
            if (sent < to_send) {
                auto* begin = outgoing.data() + sent;
                auto* end = outgoing.data() + to_send;

                std::copy(begin, end, std::back_inserter(untransmitted));
                total_sent += sent;
                break;
            }
        }

        assert(sent == to_send);
        total_sent += sent;
    }

    OP_LOG(OP_LOG_TRACE,
           "Transmitted %u of %u packets on %u:%u from source %s\n",
           total_sent,
           total_to_send,
           port_idx,
           queue_idx,
           source->id().c_str());

    return (total_sent);
}

template <typename Source>
static void do_drop(uint16_t port_idx,
                    uint16_t queue_idx,
                    Source& source,
                    std::vector<rte_mbuf*>& mbufs)
{
    auto octets = std::accumulate(std::begin(mbufs),
                                  std::end(mbufs),
                                  size_t{0},
                                  [](size_t sum, const auto* mbuf) {
                                      return (sum + rte_pktmbuf_pkt_len(mbuf));
                                  });

    source->update_drop_counters(mbufs.size(), octets);

    OP_LOG(OP_LOG_TRACE,
           "Dropping %zu packets on %u:%u from source %s\n",
           mbufs.size(),
           port_idx,
           queue_idx,
           source->id().c_str());

    rte_pktmbuf_free_bulk(mbufs.data(), mbufs.size());
    mbufs.clear();
}

std::optional<schedule::state>
tx_scheduler::on_timeout(const schedule::state_running& state)
{
    assert(m_buffer.empty());

    if (link_down(port_id())) return (schedule::state_link_check{});

    if (state.reschedule) {
        do_reschedule(schedule::clock::now());
        state.reschedule = false;
    }

    schedule::time_point now;
    while (!m_schedule.empty()
           && m_schedule.top().deadline <= (now = schedule::clock::now())) {
        /* Grab the event from the schedule */
        auto [deadline, key] = m_schedule.top();
        m_schedule.pop();

        /*
         * Verify the source is still in our forwarding table;
         * it could have been removed while we were idle.
         */
        auto source = m_tib.get_source(key);
        if (!source) continue;

        auto burst_size = source->burst_size();
        auto sent =
            do_transmit(port_id(), queue_id(), source, burst_size, m_buffer);

        if (!m_buffer.empty()) {
            /*
             * If the user wants us to drop overruns, do that here. Otherwise
             * transition to the blocked state.
             */
            if (config::dpdk_drop_tx_overruns()) {
                do_drop(port_id(), queue_id(), source, m_buffer);
            } else {
                uint16_t remaining = burst_size - sent - m_buffer.size();
                return (schedule::state_blocked{remaining, {deadline, key}});
            }
        }

        /* Re-add entry to schedule if it still active */
        if (source->active()) {
            /*
             * Always update from the previous deadline value, as we don't want
             * our intervals to drift.
             */
            m_schedule.push({deadline + next_deadline(*source), key});
        }
    }

    if (m_schedule.empty()) return (schedule::state_idle{});

    assert(m_schedule.top().deadline > now);

    /* Update timer for next event */
    if (m_time_reschedule <= m_schedule.top().deadline) {
        set_timer_oneshot(m_timerfd,
                          std::max(min_poll, m_time_reschedule - now));
        state.reschedule = true;
    } else {
        set_timer_oneshot(m_timerfd, m_schedule.top().deadline - now);
    }

    return (std::nullopt);
}

std::optional<schedule::state>
tx_scheduler::on_timeout(const schedule::state_blocked& blocked)
{
    assert(!m_buffer.empty());
    assert(!config::dpdk_drop_tx_overruns());

    if (link_down(port_id())) return (schedule::state_link_check{});

    auto& [deadline, key] = blocked.entry;

    /*
     * Verify source still exists.  If it doesn't clear the buffer and
     * transition to the appropriate state.
     */
    auto source = m_tib.get_source(key);
    if (!source) {
        m_buffer.clear();
        if (have_active_sources(m_tib, port_id(), queue_id())) {
            return (schedule::state_running{});
        } else {
            return (schedule::state_idle{});
        }
    }

    auto to_send = m_buffer.size();

    auto nb_prep =
        rte_eth_tx_prepare(port_id(), queue_id(), m_buffer.data(), to_send);
    if (nb_prep != to_send) {
        OP_LOG(OP_LOG_DEBUG,
               "Failed preparing packets on %u:%u.  Prepared %hu of %lu "
               "packets.  %s.\n",
               port_id(),
               queue_id(),
               nb_prep,
               to_send,
               rte_strerror(rte_errno));
        return (std::nullopt);
    }

    auto sent =
        rte_eth_tx_burst(port_id(), queue_id(), m_buffer.data(), to_send);
    if (sent < to_send) {
        m_buffer.erase(std::begin(m_buffer), std::begin(m_buffer) + sent);
        set_timer_oneshot(m_timerfd, block_poll);
        return (std::nullopt);
    }
    assert(sent == to_send);
    m_buffer.clear();

    sent =
        do_transmit(port_id(), queue_id(), source, blocked.remaining, m_buffer);

    /* Still blocked */
    if (sent < blocked.remaining) {
        blocked.remaining -= sent;
        set_timer_oneshot(m_timerfd, block_poll);
        return (std::nullopt);
    }

    /*
     * Blocked queue is cleared.
     * Add the blocked source back to the schedule if it is still active.
     * Note: we use the current time as the next source deadline so that
     * we don't generate new events in the past.
     */
    assert(sent == blocked.remaining);
    assert(m_buffer.empty());
    if (source->active()) m_schedule.push({schedule::clock::now(), key});

    /* If we don't have any active sources, return to the idle state */
    if (!have_active_sources(m_tib, port_id(), queue_id())) {
        return (schedule::state_idle{});
    }

    /*
     * Otherwise, jump back into the run state.  If the run handler doesn't
     * return an updated state, then we should stay in the run state; let
     * the upper layer know.
     */
    auto state = on_timeout(schedule::state_running{});
    return (state ? *state : schedule::state_running{});
}

void tx_scheduler::on_transition(const schedule::state_idle&)
{
    assert(m_buffer.empty());
    // assert(!have_active_sources(m_tib, port_id(), queue_id()));

    set_timer_interval(m_timerfd, idle_poll);
}

void tx_scheduler::on_transition(const schedule::state_link_check&)
{
    /* Drop any buffered packets as we can't do anything with them without a
     * link */
    std::for_each(std::begin(m_buffer), std::end(m_buffer), rte_pktmbuf_free);
    m_buffer.clear();

    /* Drop any scheduled items as we can't do anything with them without a link
     * either */
    while (!m_schedule.empty()) { m_schedule.pop(); }

    set_timer_interval(m_timerfd, link_poll);
}

void tx_scheduler::on_transition(const schedule::state_running& state)
{
    assert(m_buffer.empty());

    auto now = schedule::clock::now();
    if (m_schedule.empty()) {
        /* Generate a schedule for all available entities */
        for (auto&& pair : m_tib.get_sources(port_id(), queue_id())) {
            const auto& source = pair->second;
            if (source.active()) {
                m_schedule.push({now + next_deadline(source),
                                 worker::tib::to_safe_key(pair->first)});
            }
        }

        /* Adjust timer to match schedule */
        m_time_reschedule = now + schedule_poll;
    }

    if (m_schedule.empty() || m_time_reschedule <= m_schedule.top().deadline) {
        state.reschedule = true;
        set_timer_oneshot(m_timerfd,
                          std::max(min_poll, m_time_reschedule - now));
    } else {
        state.reschedule = false;
        set_timer_oneshot(m_timerfd,
                          std::max(min_poll, m_schedule.top().deadline - now));
    }
}

void tx_scheduler::on_transition(const schedule::state_blocked&)
{
    assert(!m_buffer.empty());
    assert(!config::dpdk_drop_tx_overruns());
    set_timer_oneshot(m_timerfd, block_poll);
}

} // namespace openperf::packetio::dpdk
