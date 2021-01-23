#include <thread>
#include <limits>
#include <cerrno>
#include <cstdlib>
#include <cinttypes>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <tl/expected.hpp>
#include <fcntl.h>

#include "task.hpp"

#include "config/op_config_file.hpp"
#include "framework/core/op_log.h"
#include "framework/core/op_event_loop.hpp"
#include "framework/utils/random.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "firehose/protocol.hpp"
#include "utils/network_sockaddr.hpp"
#include "drivers/kernel.hpp"
#include "packet/type/ipv6_address.hpp"
#include "timesync/bintime.hpp"

namespace openperf::network::internal::task {

using namespace std::chrono_literals;
constexpr duration TASK_SPIN_THRESHOLD = 100ms;
constexpr duration QUANTA = 10ms;
constexpr auto default_operation_timeout_usec = 1000000;
const size_t max_buffer_size = 64 * 1024;

stat_t& stat_t::operator+=(const stat_t& stat)
{
    assert(operation == stat.operation);

    updated = std::max(updated, stat.updated);
    ops_target += stat.ops_target;
    ops_actual += stat.ops_actual;
    bytes_target += stat.bytes_target;
    bytes_actual += stat.bytes_actual;
    latency += stat.latency;
    errors += stat.errors;

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

    conn_stat.attempted += stat.conn_stat.attempted;
    conn_stat.successful += stat.conn_stat.successful;
    conn_stat.errors += stat.conn_stat.errors;
    conn_stat.closed += stat.conn_stat.closed;

    return *this;
}

class connection_init_state
{
public:
    static int write(const struct op_event_data* data, void* arg)
    {
        connection_t* con = reinterpret_cast<connection_t*>(arg);
        assert(con);
        if (!con) return -1;
        auto task = con->task;
        assert(task);
        return task->do_init(*con, task->m_spin_stat);
    }

    static int close(const struct op_event_data* data, void* arg)
    {
        connection_t* con = reinterpret_cast<connection_t*>(arg);
        assert(con);
        if (!con) return -1;
        auto task = con->task;
        assert(task);
        return task->do_shutdown(*con, task->m_spin_stat);
    }

    static constexpr op_event_callbacks callbacks = {
        .on_write = connection_init_state::write,
        .on_delete = connection_init_state::close};
};

class connection_reading_state
{
public:
    static int read(const struct op_event_data* data, void* arg)
    {
        connection_t* con = reinterpret_cast<connection_t*>(arg);
        assert(con);
        if (!con) return -1;
        auto task = con->task;
        assert(task);
        return task->do_read(*con, task->m_spin_stat);
    }

    static int close(const struct op_event_data* data, void* arg)
    {
        connection_t* con = reinterpret_cast<connection_t*>(arg);
        assert(con);
        if (!con) return -1;
        auto task = con->task;
        assert(task);
        return task->do_shutdown(*con, task->m_spin_stat);
    }

    static constexpr op_event_callbacks callbacks = {
        .on_read = connection_reading_state::read,
        .on_delete = connection_reading_state::close};
};

class connection_writing_state
{
public:
    static int write(const struct op_event_data* data, void* arg)
    {
        connection_t* con = reinterpret_cast<connection_t*>(arg);
        assert(con);
        if (!con) return -1;
        auto task = con->task;
        assert(task);
        return task->do_write(*con, task->m_spin_stat);
    }

    static int close(const struct op_event_data* data, void* arg)
    {
        connection_t* con = reinterpret_cast<connection_t*>(arg);
        assert(con);
        if (!con) return -1;
        auto task = con->task;
        assert(task);
        return task->do_shutdown(*con, task->m_spin_stat);
    }

    static constexpr op_event_callbacks callbacks = {
        .on_write = connection_writing_state::write,
        .on_delete = connection_writing_state::close};
};

// Constructors & Destructor
network_task::network_task(const config_t& configuration,
                           const drivers::driver_ptr& driver)
    : m_driver(driver)
    , m_loop(new core::event_loop())
{
    m_write_buffer.resize(std::min(configuration.block_size, max_buffer_size));
    utils::op_prbs23_fill(m_write_buffer.data(), m_write_buffer.size());
    config(configuration);
}

network_task::~network_task()
{
    if (m_loop) {
        if (m_loop->count()) {
            OP_LOG(OP_LOG_DEBUG, "Purging %zu connections", m_loop->count());
            m_loop->purge();

            // FIXME:
            // The worker class calls op_log_close() before exiting the thread
            // function. Unfortunatley the task object is passed by value as
            // an argument when creating the thread so this dtor is called
            // after the thread function exits.
            // So any logging here will cause log zmq sockets to leak unless
            // the log socket is closed.
            op_log_close();
        }
    }
}

network_task::network_task(network_task&& orig)
    : m_config(orig.m_config)
    , m_stat(orig.m_stat)
    , m_driver(std::move(orig.m_driver))
    , m_loop(std::move(orig.m_loop))
    , m_operation_timestamp(orig.m_operation_timestamp)
    , m_connections(std::move(orig.m_connections))
    , m_write_buffer(std::move(orig.m_write_buffer))
{
    for (auto& it : m_connections) { it.second->task = this; }
}

// Methods : public
void network_task::reset()
{
    m_stat = {.operation = m_config.operation};
    m_operation_timestamp = {};
    if (m_config.synchronizer) {
        switch (m_config.operation) {
        case operation_t::READ:
            m_config.synchronizer->reads_actual.store(
                0, std::memory_order_relaxed);
            break;
        case operation_t::WRITE:
            m_config.synchronizer->writes_actual.store(
                0, std::memory_order_relaxed);
            break;
        }
    }
}

static tl::expected<network_sockaddr, int>
populate_sockaddr(const drivers::driver_ptr& driver,
                  const std::string& host,
                  in_port_t port,
                  const std::optional<std::string>& interface)
{
    sockaddr_storage client_storage;
    auto* sa4 = reinterpret_cast<sockaddr_in*>(&client_storage);
    auto* sa6 = reinterpret_cast<sockaddr_in6*>(&client_storage);

    if (inet_pton(AF_INET, host.c_str(), &sa4->sin_addr) == 1) {
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons(port);
        return network_sockaddr_assign(reinterpret_cast<sockaddr*>(sa4));
    } else if (inet_pton(AF_INET6, host.c_str(), &sa6->sin6_addr) == 1) {
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(port);
        using namespace libpacket::type;
        if (is_linklocal(ipv6_address(host))) {
            /* Need to set sin6_scope_id for link local IPv6 */
            int ifindex;
            if ((ifindex =
                     driver->if_nametoindex(interface.value_or("").c_str()));
                ifindex > 0) {
                sa6->sin6_scope_id = ifindex;
            } else {
                OP_LOG(OP_LOG_WARNING,
                       "Unable to find interface for link local %s\n",
                       host.c_str());
            }
        }
        return network_sockaddr_assign(reinterpret_cast<sockaddr*>(sa6));
    }

    /* host is not a valid IPv4 or IPv6 address; fail */
    return tl::make_unexpected(EINVAL);
}

void update_stat_latency(stat_t& stat, duration dur)
{
    stat.latency += dur;
    if (!stat.latency_min || dur < stat.latency_min.value()) {
        stat.latency_min = dur;
    }
    if (!stat.latency_max || dur < stat.latency_max.value()) {
        stat.latency_max = dur;
    }
}

stat_t network_task::spin()
{
    if (!m_config.ops_per_sec || !m_config.block_size) {
        throw std::runtime_error(
            "Could not spin worker: no block operations were specified");
    }

    if (m_operation_timestamp.time_since_epoch() == 0ns) {
        m_operation_timestamp = ref_clock::now();
    }

    // Reduce the effect of ZMQ operations on total Block I/O operations amount
    auto sleep_time = m_operation_timestamp - ref_clock::now();
    if (sleep_time > 0ns) std::this_thread::sleep_for(sleep_time);

    // Worker loop
    auto stat = stat_t{.operation = m_config.operation};
    auto cur_time = ref_clock::now();
    auto loop_start_ts = cur_time;

    // Prepare for ratio synchronization
    auto ops_per_sec = calculate_rate();

    do {
        // +1 need here to start spin at beginning of each time frame
        uint64_t dt = (cur_time - m_operation_timestamp).count();
        auto ops_req = dt * ops_per_sec / std::nano::den + 1;

        assert(ops_req);
        auto worker_spin_stat = worker_spin(ops_req);
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

tl::expected<connection_ptr, int>
network_task::new_connection(const network_sockaddr& server,
                             const config_t& config)
{
    int sock = 0;
    if ((sock = m_driver->socket(
             network_sockaddr_family(server),
             config.target.protocol == IPPROTO_TCP ? SOCK_STREAM : SOCK_DGRAM,
             config.target.protocol))
        == -1) {
        return tl::make_unexpected(errno);
    }

    if (config.target.interface) {
        if (m_driver->setsockopt(sock,
                                 SOL_SOCKET,
                                 SO_BINDTODEVICE,
                                 config.target.interface.value().c_str(),
                                 config.target.interface.value().size())
            < 0) {
            auto err = errno;
            m_driver->close(sock);
            return tl::make_unexpected(err);
        }
    }

#if defined(SO_NOSIGPIPE)
    int enable = 1;
    if (m_driver->setsockopt(
            sock, SOL_SOCKET, SO_NOSIGPIPE, &enable, sizeof(enable))
        != 0) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(err);
    }
#endif

    /* Update to non-blocking socket */
    int flags = m_driver->fcntl(sock, F_GETFL);
    if (flags == -1) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(err);
    }

    if (m_driver->fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(err);
    }

    auto* sa = std::visit([](auto&& sa) { return (sockaddr*)&sa; }, server);
    if (m_driver->connect(sock, sa, network_sockaddr_size(server)) == -1
        && errno != EINPROGRESS) {
        /* not what we were expecting */
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(err);
    }

#if !defined(SO_NOSIGPIPE)
    int enable = 1;
#else
    enable = 1;
#endif
    /* Disable the Nagle algorithm as delayed ack is typically enabled. The two
     * algorithms do not play well together. On a single connection, rate would
     * be limited to 10 operations per second when both read/write are enabled
     * and the write didn't fill the maximum segment size. Nagle would wait for
     * the ACK that the peer decided to delay. So we wouldn't resume tranmition
     * until the delayed ack timer fired (100ms on BSD, 40ms on Linux).
     */
    if ((config.target.protocol == IPPROTO_TCP)
        && (m_driver->setsockopt(
                sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable))
            != 0)) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(err);
    }

    if (config.target.protocol == IPPROTO_UDP) {
        static auto timeout = std::chrono::microseconds(
            openperf::config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
                "modules.network.operation-timeout")
                .value_or(default_operation_timeout_usec));

        static auto read_timeout = timesync::to_timeval(timeout);
        m_driver->setsockopt(
            sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    }

    return std::unique_ptr<connection_t>(new connection_t{
        .fd = sock,
        .state = STATE_INIT,
        .buffer =
            std::vector<uint8_t>(std::min(config.block_size, max_buffer_size)),
        .bytes_left = 0,
        .ops_left = config.ops_per_connection,
        .task = this});
}

int network_task::do_init(connection_t& conn, stat_t& stat)
{
    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    if (m_spin_ops_left == 0) {
        m_loop->exit();
        return 0;
    }

    if (conn.ops_left == 0) {
        conn.state = STATE_DONE;
        return -1;
    }

    firehose::request_t to_request = {
        .action = (m_config.operation == operation_t::READ)
                      ? firehose::action_t::GET
                      : firehose::action_t::PUT,
        .length = static_cast<uint32_t>(m_config.block_size),
    };

    std::vector<uint8_t> header;
    auto r = firehose::build_request(to_request, header);
    if (!r) {
        OP_LOG(OP_LOG_ERROR,
               "Cannot build firehose request: %s",
               strerror(r.error()));
        return -1;
    }

    conn.operation_start_time = ref_clock::now();

    switch (m_config.operation) {
    case operation_t::READ: {
        ssize_t send_or_err =
            m_driver->send(conn.fd, header.data(), header.size(), flags);
        if (send_or_err == -1 || send_or_err < (ssize_t)header.size()) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }

            conn.state = STATE_ERROR;
            return -1;
        }

        conn.state = STATE_READING;
        conn.bytes_left = m_config.block_size;
        m_loop->update(conn.fd, &connection_reading_state::callbacks);
    } break;
    case operation_t::WRITE: {
        iovec iov[] = {
            {
                .iov_base = header.data(),
                .iov_len = header.size(),
            },
            {
                .iov_base = m_write_buffer.data(),
                .iov_len = std::max(m_write_buffer.size(),
                                    m_config.block_size - header.size()),
            },
        };

        msghdr to_send = {
            .msg_iov = iov,
            .msg_iovlen = 2,
        };
        ssize_t send_or_err = m_driver->sendmsg(conn.fd, &to_send, flags);
        /*
         * Obviously check to see if we could send our data.
         * Non-obviously, make sure we could get the full request header out.
         * Failing to send less than a full request header should be a rare
         * event and it's easier to just close the connection that recover
         * from it.
         */
        if (send_or_err == -1 || send_or_err < (ssize_t)header.size()) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }

            conn.state = STATE_ERROR;
            return -1;
        }

        conn.bytes_left = header.size() + m_config.block_size - send_or_err;
        if (conn.bytes_left) {
            conn.state = STATE_WRITING;
            m_loop->update(conn.fd, &connection_writing_state::callbacks);
        } else {
            stat.bytes_actual += send_or_err;
            stat.ops_actual++;
            update_stat_latency(stat,
                                ref_clock::now() - conn.operation_start_time);
            conn.ops_left--;
            m_spin_ops_left--;
            if (conn.ops_left > 0) {
                conn.state = STATE_INIT;
                m_loop->update(conn.fd, &connection_init_state::callbacks);
            } else {
                conn.state = STATE_DONE;
                return -1;
            }
        }

    } break;
    }

    return 0;
}

int network_task::do_read(connection_t& conn, stat_t& stat)
{
    assert(conn.state == STATE_READING);
    ssize_t recv_or_err = 0;
    while (conn.bytes_left) {
        if ((recv_or_err =
                 m_driver->recv(conn.fd,
                                conn.buffer.data(),
                                std::min(conn.buffer.size(), conn.bytes_left),
                                0))
            == -1) {
            static auto timeout = std::chrono::microseconds(
                openperf::config::file::op_config_get_param<
                    OP_OPTION_TYPE_LONG>("modules.network.operation-timeout")
                    .value_or(default_operation_timeout_usec));
            if ((errno == EAGAIN || errno == EWOULDBLOCK)
                && (ref_clock::now() - conn.operation_start_time < timeout)) {
                return 0;
            }
            conn.state = STATE_ERROR;
            return -1;
        } else if (recv_or_err == 0) {
            conn.state = STATE_DONE;
            return -1; /* remote side closed connection */
        } else {
            conn.bytes_left -= recv_or_err;
        }
    }

    stat.ops_actual++;
    conn.ops_left--;
    m_spin_ops_left--;
    stat.bytes_actual += m_config.block_size;
    update_stat_latency(stat, ref_clock::now() - conn.operation_start_time);

    conn.state = STATE_INIT;
    m_loop->update(conn.fd, &connection_init_state::callbacks);

    return 0;
}

int network_task::do_write(connection_t& conn, stat_t& stat)
{
    assert(conn.state == STATE_WRITING);

    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    while (conn.bytes_left) {
        ssize_t send_or_err =
            m_driver->send(conn.fd,
                           m_write_buffer.data(),
                           std::min(conn.bytes_left, m_write_buffer.size()),
                           flags);
        if (send_or_err == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }

            conn.state = STATE_ERROR;
            return -1;
        }
        conn.bytes_left -= send_or_err;
    }

    assert(conn.bytes_left == 0);

    stat.ops_actual++;
    conn.ops_left--;
    m_spin_ops_left--;
    stat.bytes_actual += m_config.block_size;
    update_stat_latency(stat, ref_clock::now() - conn.operation_start_time);

    conn.state = STATE_INIT;
    m_loop->update(conn.fd, &connection_init_state::callbacks);
    return 0;
}

int network_task::do_shutdown(connection_t& conn, stat_t& stat)
{
    if (conn.state == STATE_ERROR) stat.errors++;

    m_driver->close(conn.fd);
    stat.conn_stat.closed++;
    m_connections.erase(conn.fd);
    return 0;
}

// Methods : private
stat_t network_task::worker_spin(uint64_t nb_ops)
{
    // Clear spin stats
    m_spin_stat = stat_t{.operation = m_config.operation};
    m_spin_ops_left = nb_ops;

    /* Make sure we have the right number of connections */
    size_t connections_required = m_config.connections - m_connections.size();
    for (size_t c = 0; c < connections_required; c++) {
        auto server = populate_sockaddr(m_driver,
                                        m_config.target.host,
                                        m_config.target.port,
                                        m_config.target.interface);
        if (!server) {
            OP_LOG(OP_LOG_TRACE,
                   "Could not open new connection: %s\n",
                   strerror(abs(server.error())));
            m_spin_stat.conn_stat.errors++;
            break;
        }
        m_spin_stat.conn_stat.attempted++;
        auto conn = new_connection(server.value(), m_config);
        if (!conn) {
            OP_LOG(OP_LOG_TRACE,
                   "Could not open new connection: %s\n",
                   strerror(abs(conn.error())));
            m_spin_stat.conn_stat.errors++;
            break;
        }
        m_spin_stat.conn_stat.successful++;
        m_loop->add(conn.value()->fd,
                    &connection_init_state::callbacks,
                    reinterpret_cast<void*>(conn.value().get()));
        m_connections[conn.value()->fd] = std::move(conn.value());
    }

    m_loop->run(
        std::chrono::duration_cast<std::chrono::milliseconds>(QUANTA).count());

    return m_spin_stat;
}

void network_task::config(const config_t& p_config)
{
    m_config = p_config;
    m_stat.operation = m_config.operation;
}

int32_t network_task::calculate_rate()
{
    if (!m_config.synchronizer) return m_config.ops_per_sec;

    if (m_config.operation == operation_t::READ)
        m_config.synchronizer->reads_actual.store(m_stat.ops_actual,
                                                  std::memory_order_relaxed);
    else
        m_config.synchronizer->writes_actual.store(m_stat.ops_actual,
                                                   std::memory_order_relaxed);

    int64_t reads_actual =
        m_config.synchronizer->reads_actual.load(std::memory_order_relaxed);
    int64_t writes_actual =
        m_config.synchronizer->writes_actual.load(std::memory_order_relaxed);

    int32_t ratio_reads =
        m_config.synchronizer->ratio_reads.load(std::memory_order_relaxed);
    int32_t ratio_writes =
        m_config.synchronizer->ratio_writes.load(std::memory_order_relaxed);

    switch (m_config.operation) {
    case operation_t::READ: {
        auto reads_expected = writes_actual * ratio_reads / ratio_writes;
        return std::min(
            std::max(reads_expected + static_cast<int64_t>(m_config.ops_per_sec)
                         - reads_actual,
                     1L),
            static_cast<long>(m_config.ops_per_sec));
    }
    case operation_t::WRITE: {
        auto writes_expected = reads_actual * ratio_writes / ratio_reads;
        return std::min(
            std::max(writes_expected
                         + static_cast<int64_t>(m_config.ops_per_sec)
                         - writes_actual,
                     1L),
            static_cast<long>(m_config.ops_per_sec));
    }
    }
}

} // namespace openperf::network::internal::task
