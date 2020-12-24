#include <cassert>
#include <stdexcept>
#include <fcntl.h>

#include "config/op_config_file.hpp"
#include "core/op_log.h"
#include "core/op_thread.h"
#include "timesync/bintime.hpp"
#include "utils/random.hpp"
#include "server_udp.hpp"
#include "protocol.hpp"

#include "../utils/network_sockaddr.hpp"
namespace openperf::network::internal::firehose {

constexpr auto default_operation_timeout_usec = 1000000;

tl::expected<int, std::string> server_udp::new_server(
    int domain, in_port_t port, const std::optional<std::string>& interface)
{
    struct sockaddr_storage client_storage;
    auto* server_ptr = reinterpret_cast<sockaddr*>(&client_storage);

    switch (domain) {
    case AF_INET: {
        auto* server4 = reinterpret_cast<sockaddr_in*>(server_ptr);
        server4->sin_family = AF_INET;
        server4->sin_port = htons(port);
        server4->sin_addr.s_addr = htonl(INADDR_ANY);
        break;
    }
    case AF_INET6: {
        auto* server6 = reinterpret_cast<sockaddr_in6*>(server_ptr);
        server6->sin6_family = AF_INET6;
        server6->sin6_port = htons(port);
        server6->sin6_addr = in6addr_any;
        break;
    }
    default:
        return tl::make_unexpected<std::string>(strerror(EINVAL));
    }

    int sock = 0, enable = true;
    if ((sock = m_driver->socket(domain, SOCK_DGRAM, 0)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to open %s UDP server socket: %s\n",
               (domain == AF_INET6) ? "IPv6" : "IPv4",
               strerror(errno));
        return tl::make_unexpected<std::string>(strerror(errno));
    }

    if (interface) {
        if (m_driver->setsockopt(sock,
                                 SOL_SOCKET,
                                 SO_BINDTODEVICE,
                                 interface.value().c_str(),
                                 interface.value().size())
            < 0) {
            auto err = errno;
            m_driver->close(sock);
            return tl::make_unexpected<std::string>(strerror(err));
        }
    }

    if (m_driver->setsockopt(
            sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))
        != 0) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    /* Update to non-blocking socket */
    int flags = m_driver->fcntl(sock, F_GETFL);
    if (flags == -1) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(strerror(err));
    }

    if (m_driver->fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected(strerror(err));
    }

    if (m_driver->bind(sock, server_ptr, get_sa_len(server_ptr)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to bind to socket (domain = %d, protocol = UDP): %s\n",
               domain,
               strerror(errno));
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    static auto timeout = std::chrono::microseconds(
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
            "modules.network.operation-timeout")
            .value_or(default_operation_timeout_usec));

    static auto read_timeout = timesync::to_timeval(timeout);
    if (m_driver->setsockopt(
            sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout))
        != 0) {
        OP_LOG(OP_LOG_WARNING,
               "Unable to set socket timeout (domain = %d, protocol = "
               "UDP): %s\n",
               domain,
               strerror(errno));
    }

    return sock;
}

server_udp::server_udp(in_port_t port,
                       const std::optional<std::string>& interface,
                       const drivers::driver_ptr& driver)
    : m_stopped(false)
{
    m_driver = driver;

    /* IPv6 any supports IPv4 and IPv6 */
    tl::expected<int, std::string> res;
    if ((res = new_server(AF_INET6, port, interface)); res) {
        OP_LOG(OP_LOG_DEBUG, "Network UDP load server IPv4/IPv6.\n");
    } else if ((res = new_server(AF_INET, port, interface)); res) {
        OP_LOG(OP_LOG_DEBUG, "Network UDP load server IPv4.\n");
    } else {
        throw std::runtime_error("Cannot create UDP server: " + res.error());
    }

    m_fd = res.value();

    run_worker_thread();
}

server_udp::~server_udp()
{
    m_stopped.store(true, std::memory_order_relaxed);
    if (m_fd.load() >= 0) { m_driver->close(m_fd); }

    if (m_worker_thread.joinable()) m_worker_thread.join();
}

// One worker for udp connections
void server_udp::run_worker_thread()
{
    m_worker_thread = std::thread([&] {
        // Set the thread name
        op_thread_setname("op_net_srv_w");

        std::vector<connection_t> connections;
        ssize_t recv_or_err = 0;
        connection_t conn;
        sockaddr_storage client_storage;
        auto* client = reinterpret_cast<sockaddr*>(&client_storage);
        socklen_t client_length = sizeof(struct sockaddr_in6);

        std::vector<uint8_t> read_buffer(net_request_size);
        std::vector<uint8_t> send_buffer(send_buffer_size);
        utils::op_prbs23_fill(send_buffer.data(), send_buffer.size());

        while (!m_stopped.load(std::memory_order_relaxed)) {
            // Receive all accepted connections
            while ((recv_or_err = m_driver->recvfrom(m_fd,
                                                     read_buffer.data(),
                                                     read_buffer.size(),
                                                     0,
                                                     client,
                                                     &client_length))
                   != -1) {
                uint8_t* recv_cursor = read_buffer.data();
                size_t bytes_left = recv_or_err;
                m_stat.bytes_received += recv_or_err;
                m_stat.connections++;

                auto req = firehose::parse_request(recv_cursor, bytes_left);
                if (!req) {
                    char ntopbuf[INET6_ADDRSTRLEN];
                    const char* addr = inet_ntop(client->sa_family,
                                                 get_sa_addr(client),
                                                 ntopbuf,
                                                 INET6_ADDRSTRLEN);
                    OP_LOG(OP_LOG_ERROR,
                           "Invalid firehose request received "
                           "from %s:%d\n",
                           addr ? addr : "unknown",
                           ntohs(get_sa_port(client)));
                    OP_LOG(OP_LOG_ERROR,
                           "recv = %zu, bytes_left = %zu\n",
                           recv_or_err,
                           bytes_left);
                    continue;
                }
                conn.state = (req.value().action == action_t::GET)
                                 ? STATE_WRITING
                                 : STATE_READING;
                conn.bytes_left = req.value().length;
                bytes_left -= firehose::net_request_size;
                recv_cursor += firehose::net_request_size;

                while (conn.state == STATE_WRITING && conn.bytes_left) {
                    size_t produced =
                        std::min(send_buffer.size(), conn.bytes_left);
                    ssize_t send_or_err = m_driver->sendto(m_fd,
                                                           send_buffer.data(),
                                                           produced,
                                                           0,
                                                           client,
                                                           client_length);
                    if (send_or_err == -1) {
                        conn.state = STATE_ERROR;
                        conn.bytes_left = 0;
                        m_stat.errors++;
                    } else {
                        conn.bytes_left -= send_or_err;
                        m_stat.bytes_sent += send_or_err;
                    }
                }
                m_stat.closed++;
            }
        }
    });
}

} // namespace openperf::network::internal::firehose
