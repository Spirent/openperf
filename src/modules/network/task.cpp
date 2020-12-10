#include <thread>
#include <limits>
#include <cerrno>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <tl/expected.hpp>
#include <fcntl.h>

#include "task.hpp"

#include "framework/core/op_log.h"
#include "framework/utils/random.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "firehose/protocol.hpp"
#include "utils/network_sockaddr.hpp"
#include "utils/ipv6.hpp"
#include "drivers/kernel.hpp"

namespace openperf::network::internal::task {

using ref_clock = timesync::chrono::monotime;
using namespace std::chrono_literals;
constexpr duration TASK_SPIN_THRESHOLD = 100ms;
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

// Constructors & Destructor
network_task::network_task(const config_t& configuration,
                           const drivers::driver_ptr& driver)
    : m_driver(driver)
{
    m_write_buffer.resize(std::min(configuration.block_size, max_buffer_size));
    utils::op_prbs23_fill(m_write_buffer.data(), m_write_buffer.size());
    config(configuration);
}

network_task::~network_task()
{
    for (auto& conn : m_connections) { do_shutdown(conn, m_stat); }
    m_connections.clear();
}

// Methods : public
void network_task::reset()
{
    m_stat = {.operation = m_config.operation};
    m_operation_timestamp = {};
}

static int
populate_sockaddr(network_sockaddr& s, const std::string& host, uint16_t port)
{
    sockaddr_storage client_storage;
    auto sa4 = (sockaddr_in*)&client_storage;
    auto sa6 = (sockaddr_in6*)&client_storage;

    if (inet_pton(AF_INET, host.c_str(), &sa4->sin_addr) == 1) {
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons(port);
        network_sockaddr_assign((sockaddr*)sa4, s);
    } else if (inet_pton(AF_INET6, host.c_str(), &sa6->sin6_addr) == 1) {
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(port);
        if (ipv6_addr_is_link_local(&sa6->sin6_addr)) {
            /* Need to set sin6_scope_id for link local IPv6 */
            int ifindex;
            if ((ifindex = ipv6_get_neighbor_ifindex(&sa6->sin6_addr)) >= 0) {
                sa6->sin6_scope_id = ifindex;
            } else {
                OP_LOG(OP_LOG_WARNING,
                       "Unable to find interface for link local %s\n",
                       host.c_str());
            }
        }
        network_sockaddr_assign((sockaddr*)sa6, s);
    } else {
        /* host is not a valid IPv4 or IPv6 address; fail */
        return (-EINVAL);
    }

    return (0);
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
    auto loop_start_ts = ref_clock::now();
    auto cur_time = ref_clock::now();
    do {
        // +1 need here to start spin at beginning of each time frame
        auto ops_req = (cur_time - m_operation_timestamp).count()
                           * m_config.ops_per_sec / std::nano::den
                       + 1;

        assert(ops_req);
        auto worker_spin_stat = worker_spin(ops_req);
        stat += worker_spin_stat;

        cur_time = ref_clock::now();

        m_operation_timestamp +=
            std::chrono::nanoseconds(std::nano::den / m_config.ops_per_sec)
            * worker_spin_stat.ops_actual;

    } while ((m_operation_timestamp < cur_time)
             && (cur_time <= loop_start_ts + TASK_SPIN_THRESHOLD));

    stat.updated = realtime::now();
    m_stat += stat;

    return stat;
}

tl::expected<connection_t, int>
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

    auto sa = std::visit([](auto&& sa) { return (sockaddr*)&sa; }, server);
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
        static timeval read_timeout = {
            .tv_sec = 1,
            .tv_usec = 0,
        };
        m_driver->setsockopt(
            sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    }

    return connection_t{
        .fd = sock,
        .state = STATE_INIT,
        .ops_left = config.ops_per_connection,
        .bytes_left = 0,
        .buffer =
            std::vector<uint8_t>(std::min(config.block_size, max_buffer_size)),
    };
}

void network_task::do_init(connection_t& conn, stat_t& stat)
{
    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

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
        return;
    }

    conn.operation_start_time = ref_clock::now();

    switch (m_config.operation) {
    case operation_t::READ:
        m_driver->send(conn.fd, header.data(), header.size(), flags);
        conn.state = STATE_READING;
        conn.bytes_left = m_config.block_size;
        break;
    case operation_t::WRITE:
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }

            conn.state = STATE_ERROR;
            return;
        }

        conn.bytes_left = header.size() + m_config.block_size - send_or_err;
        if (conn.bytes_left) {
            conn.state = STATE_WRITING;
        } else {
            stat.bytes_actual += send_or_err;
            stat.ops_actual++;
            update_stat_latency(stat,
                                ref_clock::now() - conn.operation_start_time);
            conn.ops_left--;
            conn.state = STATE_INIT;
        }

        break;
    }
}

void network_task::do_read(connection_t& conn, stat_t& stat)
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }

            OP_LOG(OP_LOG_ERROR,
                   "Error reading from %d: %s\n",
                   conn.fd,
                   strerror(errno));
            conn.state = STATE_ERROR;
            stat.errors++;
            return;
        } else if (recv_or_err == 0) {
            conn.state = STATE_DONE;
            return; /* remote side closed connection */
        } else {
            conn.bytes_left -= recv_or_err;
        }
    }

    stat.ops_actual++;
    conn.ops_left--;
    stat.bytes_actual += m_config.block_size;
    update_stat_latency(stat, ref_clock::now() - conn.operation_start_time);

    conn.state = STATE_INIT;

    return;
}

void network_task::do_write(connection_t& conn, stat_t& stat)
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }

            conn.state = STATE_ERROR;
            return;
        }
        conn.bytes_left -= send_or_err;
    }

    assert(conn.bytes_left == 0);

    stat.ops_actual++;
    conn.ops_left--;
    stat.bytes_actual += m_config.block_size;
    update_stat_latency(stat, ref_clock::now() - conn.operation_start_time);

    conn.state = STATE_INIT;

    return;
}

void network_task::do_shutdown(connection_t& conn, stat_t& stat)
{
    if (conn.state == STATE_ERROR) stat.errors++;

    m_driver->close(conn.fd);
    stat.conn_stat.closed++;
}

// Methods : private
stat_t network_task::worker_spin(uint64_t nb_ops)
{
    stat_t stat{.operation = m_config.operation};

    /* Make sure we have the right number of connections */
    size_t connections_required = m_config.connections - m_connections.size();
    for (size_t c = 0; c < connections_required; c++) {
        network_sockaddr server;
        populate_sockaddr(server, m_config.target.host, m_config.target.port);
        stat.conn_stat.attempted++;
        auto conn = new_connection(server, m_config);
        if (!conn) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not open new connection: %s\n",
                   strerror(abs(conn.error())));
            stat.conn_stat.errors++;
            break;
        }
        stat.conn_stat.successful++;
        m_connections.push_back(std::move(conn.value()));
    }

    for (size_t i = 0; i < nb_ops && i < m_connections.size(); i++) {
        auto& conn = m_connections[i];
        if (conn.state == STATE_INIT) do_init(conn, stat);

        if (conn.state == STATE_READING) {
            do_read(conn, stat);
        } else if (conn.state == STATE_WRITING) {
            do_write(conn, stat);
        }

        if (conn.ops_left == 0) conn.state = STATE_DONE;

        if (conn.state == STATE_DONE || conn.state == STATE_ERROR) {
            do_shutdown(conn, stat);
        }
    }

    // Remove closed connections
    m_connections.erase(std::remove_if(m_connections.begin(),
                                       m_connections.end(),
                                       [](const auto& conn) {
                                           return conn.state == STATE_DONE
                                                  || conn.state == STATE_ERROR;
                                       }),
                        m_connections.end());
    return stat;
}

void network_task::config(const config_t& p_config)
{
    m_config = p_config;
    m_stat.operation = m_config.operation;
}

} // namespace openperf::network::internal::task
