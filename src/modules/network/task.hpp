#ifndef _OP_NETWORK_GENERATOR_TASK_HPP_
#define _OP_NETWORK_GENERATOR_TASK_HPP_

#include <atomic>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"

#include "internal_utils.hpp"
#include "drivers/driver.hpp"

namespace openperf::network::internal::task {

using realtime = timesync::chrono::realtime;
using time_point = realtime::time_point;
using duration = std::chrono::nanoseconds;

enum class operation_t : uint8_t { READ = 0, WRITE };

struct target_t
{
    std::string host;
    uint64_t port;
    int protocol;
};

struct config_t
{
    operation_t operation;
    uint64_t ops_per_sec;
    uint64_t connections;
    uint64_t ops_per_connection;
    uint64_t block_size;
    target_t target;
};

enum connection_state_t {
    STATE_NONE = 0, /**< Invalid state */
    STATE_INIT,     /**< Initial connection state (requests can be made) */
    STATE_READING,  /**< Receiving transaction data state */
    STATE_WRITING,  /**< Sending transaction data state */
    STATE_ERROR,    /**< Connection has encountered an error */
    STATE_DONE      /**< Transaction limit reached; connection should close */
};
struct connection_t
{
    int fd;
    connection_state_t state;
    std::vector<uint8_t> buffer;
    size_t bytes_left;
    uint_fast64_t ops_left;
};

struct conn_stat_t
{
    uint_fast64_t attempted = 0;
    uint_fast64_t successful = 0;
    uint_fast64_t errors = 0;
    uint_fast64_t closed = 0;
};

struct stat_t
{
    using optional_time_t = std::optional<duration>;
    /**
     * updated      - Date of the last result update
     * ops_target   - The intended number of operations performed
     * ops_actual   - The actual number of operations performed
     * bytes_target - The intended number of bytes read or written
     * bytes_actual - The actual number of bytes read or written
     * errors       - The number of io_errors observed during reading or writing
     * latency      - The total amount of time required to perform operations
     * latency_min  - The minimum observed latency value
     * latency_max  - The maximum observed latency value
     */

    operation_t operation;
    conn_stat_t conn_stat;
    time_point updated = realtime::now();
    uint_fast64_t ops_target = 0;
    uint_fast64_t ops_actual = 0;
    uint_fast64_t bytes_target = 0;
    uint_fast64_t bytes_actual = 0;
    uint_fast64_t errors = 0;
    duration latency = duration::zero();
    optional_time_t latency_min;
    optional_time_t latency_max;

    stat_t& operator+=(const stat_t&);
};

class network_task : public framework::generator::task<stat_t>
{
public:
    network_task(const config_t&, const drivers::network_driver_ptr& driver);

    config_t config() const { return m_config; }

    stat_t spin() override;
    void reset() override;

private:
    void config(const config_t&);
    void reset_spin_stat();
    tl::expected<connection_t, int>
    new_connection(const network_sockaddr& server, const config_t& config);
    void do_init(connection_t& conn, stat_t& stat);
    void do_read(connection_t& conn, stat_t& stat);
    void do_write(connection_t& conn, stat_t& stat);
    void do_shutdown(connection_t& conn, stat_t& stat);
    stat_t worker_spin(uint64_t nb_ops, time_point deadline);

    config_t m_config;
    stat_t m_stat;
    drivers::network_driver_ptr m_driver;
    realtime::time_point m_operation_timestamp;
    std::vector<connection_t> m_connections;
    std::vector<uint8_t> m_write_buffer;
};

} // namespace openperf::network::internal::task

#endif // _OP_NETWORK_GENERATOR_WORKER_HPP_