#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include <sys/resource.h>
#include <sys/time.h>

#include "rte_net.h"
#include "zmq.h"

#include "config/op_config_file.hpp"
#include "core/op_core.h"
#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"

#include "simple_counter.h"

/***
 * Analyzer/sink code
 ***/

using packet_counter = simple_counter<uint64_t>;

/* Keep per worker data cache line aligned to prevent false sharing */
struct alignas(64) sink_worker_data
{
    mutable packet_counter counter;
};

class test_sink
{
    openperf::core::uuid m_id;
    bool m_active;
    std::vector<unsigned> m_indexes;
    std::vector<sink_worker_data> m_data;

    /* Generate a vector to provide a mapping from worker id -> worker index */
    static std::vector<unsigned> make_indexes(std::vector<unsigned> ids)
    {
        std::vector<unsigned> indexes(
            *max_element(std::begin(ids), std::end(ids)));
        for (unsigned i = 0; i < ids.size(); i++) { indexes[ids[i]] = i; }
        return (indexes);
    }

public:
    test_sink(openperf::core::uuid uuid, std::vector<unsigned> rx_ids)
        : m_id(std::move(uuid))
        , m_active(true)
        , m_indexes(make_indexes(rx_ids))
        , m_data(rx_ids.size())
    {}

    test_sink(test_sink&& other)
        : m_id(other.m_id)
        , m_active(true)
        , m_indexes(std::move(other.m_indexes))
        , m_data(std::move(other.m_data))
    {}

    test_sink& operator=(test_sink&& other)
    {
        if (this != &other) {
            m_id = other.m_id;
            m_indexes = std::move(other.m_indexes);
            m_data = std::move(other.m_data);
        }
        return (*this);
    }

    std::string id() const { return (openperf::core::to_string(m_id)); }

    bool active() const { return m_active; }

    bool uses_feature([
        [maybe_unused]] openperf::packetio::packets::sink_feature_flags
                          sink_feature_flags) const
    {
        return false;
    }

    std::vector<packet_counter*> counters()
    {
        std::vector<packet_counter*> counters;
        std::transform(
            std::begin(m_data),
            std::end(m_data),
            std::back_inserter(counters),
            [](auto& data) { return (std::addressof(data.counter)); });
        return (counters);
    }

    uint16_t
    push(const openperf::packetio::packets::packet_buffer* const packets[],
         uint16_t count) const
    {
        using namespace openperf::packetio;

        if (!count) return (0);

        auto& data = m_data[m_indexes[internal::worker::get_id()]];
#if 0
        auto octets = std::accumulate(packets, packets + count, 0UL,
                                      [](auto& sum, auto& packet) {
                                          return (sum + packets::length(packet) + 24);
                                      });
        data.counter.add(count, octets);
#endif
        auto udp_packets = 0UL;
        auto udp_octets = 0UL;
        std::for_each(packets, packets + count, [&](auto& packet) {
            auto mbuf = reinterpret_cast<const rte_mbuf*>(packet);
#if 0
                          auto ptype = rte_net_get_ptype(mbuf, nullptr, (RTE_PTYPE_L2_MASK
                                                                         | RTE_PTYPE_L3_MASK
                                                                         | RTE_PTYPE_L4_MASK));
#endif
            if (mbuf->packet_type & RTE_PTYPE_L4_UDP) {
                udp_packets++;
                udp_octets += packets::length(packet) + 24;
            }
        });

        if (udp_packets) data.counter.add(udp_packets, udp_octets);
        return (count);
    }
};

/***
 * Counter reading/logging code
 ***/

struct counter_sums
{
    using clock = packet_counter::clock;
    uint64_t packets = 0;
    uint64_t octets = 0;
    std::chrono::time_point<clock> first = clock::time_point::max();
    std::chrono::time_point<clock> last = clock::time_point::min();
};

static counter_sums sum_counters(std::vector<packet_counter*>& counters)
{
    counter_sums total;

    std::for_each(std::begin(counters), std::end(counters), [&](auto counter) {
        assert(counter);
        counter_sums local;

        /*
         * The loop is to ensure that we get a consistent view
         * of the counter, as the rx thread could be updating it
         * while we are trying to read it.
         */
        do {
            local.packets = counter->count.load(std::memory_order_consume);
            local.octets = counter->total.load(std::memory_order_consume);
            local.first = std::chrono::time_point<packet_counter::clock>(
                counter->first.load(std::memory_order_consume));
            local.last = std::chrono::time_point<packet_counter::clock>(
                counter->last.load(std::memory_order_consume));
        } while (local.packets
                 != counter->count.load(std::memory_order_consume));

        total.packets += local.packets;
        total.octets += local.octets;
        total.first = std::min(total.first, local.first);
        total.last = std::max(total.last, local.last);
    });

    return (total);
}

static void log_counter_delta(counter_sums& then, counter_sums& now)
{
    using namespace std::chrono;

    auto packets = now.packets - then.packets;
    if (!packets) return;

    auto octets = now.octets - then.octets;

    auto delta = duration_cast<duration<double>>(now.last - then.last).count();
    auto runtime =
        duration_cast<duration<double>>(now.last - now.first).count();
    auto interval = std::min(delta, runtime);
    if (interval <= 0) return;

    auto pps = interval ? packets / interval : 0;
    auto Mbps = interval ? octets / interval / (1000 * 1000 / 8) : 0;

    OP_LOG(OP_LOG_INFO,
           "delta: duration %.02f, %.02f Mbps, %.02f pps\n",
           runtime,
           Mbps,
           pps);
}

static std::chrono::microseconds timeval_to_us(const timeval& tv)
{
    using namespace std::chrono;
    return (seconds(tv.tv_sec) + microseconds(tv.tv_usec));
}

static void
log_counter_totals(const counter_sums& total, rusage& start, rusage& stop)
{
    using namespace std::chrono;
    auto period = duration_cast<duration<double>>(total.last - total.first);
    auto pps = period.count() ? total.packets / period.count() : 0;
    auto mbps =
        period.count() ? total.octets / period.count() / (1000 * 1000 / 8) : 0;

    auto usr_us = timeval_to_us(stop.ru_utime) - timeval_to_us(start.ru_utime);
    auto sys_us = timeval_to_us(stop.ru_stime) - timeval_to_us(start.ru_stime);
    auto cpu_us = usr_us + sys_us;

    auto period_us = duration_cast<microseconds>(period);
    auto usr_pct =
        period_us.count() ? (100.0 * usr_us.count()) / period_us.count() : 0;
    auto sys_pct =
        period_us.count() ? (100.0 * sys_us.count()) / period_us.count() : 0;
    auto cpu_pct =
        period_us.count() ? (100.0 * cpu_us.count()) / period_us.count() : 0;

    OP_LOG(OP_LOG_INFO,
           "totals: duration = %.02f seconds, packets = %zu (%.02f pps, %.02f "
           "Mbps), "
           "CPU usage: %.02f%% user/%.02f%% sys/%.02f%% total\n",
           period.count(),
           total.packets,
           pps,
           mbps,
           usr_pct,
           sys_pct,
           cpu_pct);
}

/***
 * Analyzer creation/deltion code
 ***/

using sink_data_pair = std::pair<openperf::packetio::packets::generic_sink,
                                 std::vector<packet_counter*>>;

sink_data_pair create_sink(void* context, std::string_view port_id)
{
    auto client = openperf::packetio::internal::api::client(context);

    auto rx_ids = client.get_worker_rx_ids(port_id);
    auto sink = openperf::packetio::packets::generic_sink(
        test_sink(openperf::core::uuid::random(), *rx_ids));
    auto counters = sink.get<test_sink>().counters();
    auto success = client.add_sink(port_id, sink);
    if (!success) {
        throw std::runtime_error("Could not add sink to " + std::string(port_id)
                                 + ".  Does that port exist?");
    }

    return (std::make_pair(sink, counters));
}

void delete_sink(void* context,
                 std::string_view port_id,
                 openperf::packetio::packets::generic_sink& sink)
{
    auto client = openperf::packetio::internal::api::client(context);
    client.del_sink(port_id, sink);
}

/***
 * Begin main program here
 ***/

static std::atomic_bool op_done = false;

void signal_handler(int signo __attribute__((unused))) { op_done = true; }

int main(int argc, char* argv[])
{
    op_thread_setname("op_main");

    /* Block child threads from intercepting SIGINT or SIGTERM */
    sigset_t newset, oldset;
    sigemptyset(&newset);
    sigaddset(&newset, SIGINT);
    sigaddset(&newset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    void* context = zmq_ctx_new();
    if (!context) { op_exit("Could not initialize ZeroMQ context!"); }

    op_init(context, argc, argv);

    /* Install our signal handler so we can properly shut ourselves down */
    struct sigaction s;
    s.sa_handler = signal_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
    sigaction(SIGTERM, &s, NULL);

    /* Restore the default signal mask */
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    using namespace openperf::config::file;
    auto port_id = op_config_get_param<OP_OPTION_TYPE_STRING>("main.sink-port")
                       .value_or("port0");
    auto sink_data = create_sink(context, port_id);

    /* Fire off logging thread */
    std::thread logger(
        [](auto counters) {
            op_thread_setname("op_results");

            auto start_usage = rusage{};
            getrusage(RUSAGE_SELF, &start_usage);

            auto last_sum = sum_counters(counters);
            while (!op_done) {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1s);
                auto curr_sum = sum_counters(counters);
                log_counter_delta(last_sum, curr_sum);
                last_sum = curr_sum;
            }

            auto stop_usage = rusage{};
            getrusage(RUSAGE_SELF, &stop_usage);

            log_counter_totals(sum_counters(counters), start_usage, stop_usage);
        },
        sink_data.second);

    /* Block until we're done ... */
    sigset_t emptyset;
    sigemptyset(&emptyset);
    while (!op_done) { sigsuspend(&emptyset); }

    logger.join();
    delete_sink(context, port_id, sink_data.first);

    /* ... then clean up and exit. */
    op_halt(context);

    exit(EXIT_SUCCESS);
}
