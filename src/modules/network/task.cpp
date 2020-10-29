#include <thread>
#include <limits>
#include <arpa/inet.h>

#include "task.hpp"

#include "framework/core/op_log.h"
#include "framework/utils/random.hpp"
#include "internal_utils.hpp"


namespace openperf::network::internal {

using ref_clock = timesync::chrono::monotime;
using namespace std::chrono_literals;
constexpr duration TASK_SPIN_THRESHOLD = 100ms;

struct operation_config
{
    int fd;
    size_t f_size;
    size_t block_size;
    uint8_t* buffer;
    off_t offset;
    size_t header_size;
};

task_stat_t& task_stat_t::operator+=(const task_stat_t& stat)
{
    assert(operation == stat.operation);

    updated = std::max(updated, stat.updated);
    ops_target += stat.ops_target;
    ops_actual += stat.ops_actual;
    bytes_target += stat.bytes_target;
    bytes_actual += stat.bytes_actual;
    latency += stat.latency;

    latency_min = [&]() -> optional_time_t {
        if (latency_min.has_value() && stat.latency_min.has_value())
            return std::min(latency_min.value(), stat.latency_min.value());

        return latency_min.has_value() ? latency_min : stat.latency_min;
    }();

    latency_max = [&]() -> optional_time_t {
        if (latency_max.has_value() && stat.latency_max.has_value())
            return std::max(latency_max.value(), stat.latency_max.value());

        return latency_max.has_value() ? latency_max : stat.latency_max;
    }();

    return *this;
}

// Constructors & Destructor
network_task::network_task(const task_config_t& configuration)
{
    config(configuration);
}

// Methods : public
void network_task::reset()
{
    m_stat = {.operation = m_task_config.operation};
    m_operation_timestamp = {};

    if (m_task_config.synchronizer) {
        switch (m_task_config.operation) {
        case task_operation::READ:
            m_task_config.synchronizer->reads_actual.store(
                0, std::memory_order_relaxed);
            break;
        case task_operation::WRITE:
            m_task_config.synchronizer->writes_actual.store(
                0, std::memory_order_relaxed);
            break;
        }
    }
}

static int populate_sockaddr(union network_sockaddr& s,
                                 const std::string& host,
                                 uint16_t port)
{
    if (inet_pton(AF_INET, host.c_str(), &s.sa4.sin_addr) == 1) {
        s.sa4.sin_family = AF_INET;
        s.sa4.sin_port = htons(port);
    } else if (inet_pton(AF_INET6, host.c_str(), &s.sa6.sin6_addr) == 1) {
        s.sa6.sin6_family = AF_INET6;
        s.sa6.sin6_port = htons(port);
        if (ipv6_addr_is_link_local(&s.sa6.sin6_addr)) {
            /* Need to set sin6_scope_id for link local IPv6 */
            int ifindex;
            if ((ifindex = ipv6_get_neighbor_ifindex(&s.sa6.sin6_addr))
                >= 0) {
                s.sa6.sin6_scope_id = ifindex;
            } else {
                OP_LOG(OP_LOG_WARNING,
                       "Unable to find interface for link local %s\n",
                       host.c_str());
            }
        }
    } else {
        /* host is not a valid IPv4 or IPv6 address; bail */
        return (-EINVAL);
    }

    return (0);
}

task_stat_t network_task::spin()
{
    if (!m_task_config.ops_per_sec || !m_task_config.block_size) {
        throw std::runtime_error(
            "Could not spin worker: no block operations were specified");
    }

    if (m_operation_timestamp.time_since_epoch() == 0ns) {
        m_operation_timestamp = ref_clock::now();
    }

    // Reduce the effect of ZMQ operations on total Block I/O operations amount
    auto sleep_time = m_operation_timestamp - ref_clock::now();
    if (sleep_time > 0ns) std::this_thread::sleep_for(sleep_time);

    // Prepare for ratio synchronization
    auto ops_per_sec = calculate_rate();

    // Worker loop
    auto stat = task_stat_t{.operation = m_task_config.operation};
    auto loop_start_ts = ref_clock::now();
    auto cur_time = ref_clock::now();
    do {
        // +1 need here to start spin at beginning of each time frame
        auto ops_req = (cur_time - m_operation_timestamp).count() * ops_per_sec
                           / std::nano::den
                       + 1;

        assert(ops_req);
        auto worker_spin_stat = worker_spin(ops_req, cur_time + 1s);
        stat += worker_spin_stat;

        cur_time = ref_clock::now();

        m_operation_timestamp +=
            std::chrono::nanoseconds(std::nano::den / ops_per_sec)
            * worker_spin_stat.ops_actual;

    } while ((m_operation_timestamp < cur_time)
             && (cur_time <= loop_start_ts + TASK_SPIN_THRESHOLD));

    stat.updated = realtime::now();
    m_stat += stat;

    return stat;
}

// Methods : private
task_stat_t network_task::worker_spin(uint64_t nb_ops,
                                      realtime::time_point deadline)
{
    task_stat_t stat;

    /* Make sure we have the right number of connections */
    size_t connections_required =
        m_task_config.connections - m_connections.size();
    size_t loop_conn_attempts = 0;

    while (loop_conn_attempts < connections_required) {
        network_sockaddr server;
        populate_sockaddr(server, m_task_config.host, m_task_config.port);
    }

    return stat;
}

void network_task::config(const task_config_t& p_config)
{
    m_task_config = p_config;
    m_stat.operation = m_task_config.operation;
}

int32_t network_task::calculate_rate()
{
    if (!m_task_config.synchronizer) return m_task_config.ops_per_sec;

    if (m_task_config.operation == task_operation::READ)
        m_task_config.synchronizer->reads_actual.store(
            m_stat.ops_actual, std::memory_order_relaxed);
    else
        m_task_config.synchronizer->writes_actual.store(
            m_stat.ops_actual, std::memory_order_relaxed);

    int64_t reads_actual = m_task_config.synchronizer->reads_actual.load(
        std::memory_order_relaxed);
    int64_t writes_actual = m_task_config.synchronizer->writes_actual.load(
        std::memory_order_relaxed);

    int32_t ratio_reads =
        m_task_config.synchronizer->ratio_reads.load(std::memory_order_relaxed);
    int32_t ratio_writes = m_task_config.synchronizer->ratio_writes.load(
        std::memory_order_relaxed);

    switch (m_task_config.operation) {
    case task_operation::READ: {
        auto reads_expected = writes_actual * ratio_reads / ratio_writes;
        return std::min(
            std::max(reads_expected + m_task_config.ops_per_sec - reads_actual,
                     1L),
            static_cast<long>(m_task_config.ops_per_sec));
    }
    case task_operation::WRITE: {
        auto writes_expected = reads_actual * ratio_writes / ratio_reads;
        return std::min(std::max(writes_expected + m_task_config.ops_per_sec
                                     - writes_actual,
                                 1L),
                        static_cast<long>(m_task_config.ops_per_sec));
    }
    }
}

} // namespace openperf::network::internal