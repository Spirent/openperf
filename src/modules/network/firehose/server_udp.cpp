#include <assert.h>
#include <stdexcept>

#include "core/op_log.h"
#include "core/op_thread.h"
#include "utils/random.hpp"
#include "server_udp.hpp"
#include "protocol.hpp"

namespace openperf::network::internal::firehose {

int server_udp::new_server(int domain, in_port_t port)
{
    struct sockaddr_storage client_storage;
    struct sockaddr* server_ptr = (struct sockaddr*)&client_storage;
    std::string domain_str;

    switch (domain) {
    case AF_INET: {
        struct sockaddr_in* server4 = (struct sockaddr_in*)server_ptr;
        server4->sin_family = AF_INET;
        server4->sin_port = htons(port);
        server4->sin_addr.s_addr = htonl(INADDR_ANY);
        domain_str = "IPv4";
        break;
    }
    case AF_INET6: {
        struct sockaddr_in6* server6 = (struct sockaddr_in6*)server_ptr;
        server6->sin6_family = AF_INET6;
        server6->sin6_port = htons(port);
        server6->sin6_addr = in6addr_any;
        domain_str = "IPv6";
        break;
    }
    default:
        return -EINVAL;
    }

    int sock = 0, enable = true;
    if ((sock = m_driver->socket(domain, SOCK_DGRAM, 0)) == -1) {
        OP_LOG(OP_LOG_WARNING,
               "Unable to open %s UDP server socket: %s\n",
               domain_str.c_str(),
               strerror(errno));
        return -errno;
    }

    if (m_driver->setsockopt(
            sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))
        != 0) {
        m_driver->close(sock);
        return -errno;
    }

    if (m_driver->bind(sock, server_ptr, get_sa_len(server_ptr)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to bind to socket (domain = %d, protocol = UDP): %s\n",
               domain,
               strerror(errno));
        return -errno;
    }

    return (sock);
}

server_udp::server_udp(in_port_t port,
                       const drivers::network_driver_ptr& driver)
    : m_stopped(false)
{
    m_driver = driver;
    m_driver->init();

    /* IPv6 any supports IPv4 and IPv6 */
    if ((m_fd = new_server(AF_INET6, port)) >= 0) {
        OP_LOG(OP_LOG_INFO, "Network TCP load server IPv4/IPv6.\n");
    } else {
        /* Couldn't bind IPv6 socket so use IPv4 */
        if ((m_fd = new_server(AF_INET, port)) < 0) {
            throw std::runtime_error("Cannot create UDP server: "
                                     + std::string(strerror(-m_fd)));
        }
        OP_LOG(OP_LOG_INFO, "Network TCP load server IPv4.\n");
    }

    run_worker_thread();
}

server_udp::~server_udp()
{
    m_stopped.store(true, std::memory_order_relaxed);
    if (m_fd.load() >= 0) m_driver->close(m_fd.load());

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
        socklen_t client_length = sizeof(struct sockaddr_in6);

        std::vector<uint8_t> read_buffer(net_request_size);
        OP_LOG(
            OP_LOG_INFO, "Initialized read buffer %zu\n", read_buffer.size());

        std::vector<uint8_t> send_buffer(send_buffer_size);
        utils::op_prbs23_fill(send_buffer.data(), send_buffer.size());
        OP_LOG(
            OP_LOG_INFO, "Initialized write buffer %zu\n", send_buffer.size());

        while (!m_stopped.load(std::memory_order_relaxed)) {
            // Receive all accepted connections
            while ((recv_or_err = m_driver->recvfrom(m_fd,
                                                     read_buffer.data(),
                                                     read_buffer.size(),
                                                     0,
                                                     &conn.client,
                                                     &client_length))
                   != -1) {
                uint8_t* recv_cursor = read_buffer.data();
                size_t bytes_left = recv_or_err;
                m_stat.bytes_received += recv_or_err;
                m_stat.connections++;

                auto req = firehose::parse_request(recv_cursor, bytes_left);
                if (!req) {
                    char ntopbuf[INET6_ADDRSTRLEN];
                    const char* addr = inet_ntop(conn.client.sa_family,
                                                 get_sa_addr(&conn.client),
                                                 ntopbuf,
                                                 INET6_ADDRSTRLEN);
                    OP_LOG(OP_LOG_ERROR,
                           "Invalid firehose request received "
                           "from %s:%d\n",
                           addr ? addr : "unknown",
                           ntohs(get_sa_port(&conn.client)));
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
                OP_LOG(OP_LOG_INFO,
                       "=== RECEIVED FH REQUEST %d %zu %u %s",
                       m_fd.load(),
                       recv_or_err,
                       req.value().length,
                       conn.state == STATE_WRITING ? "WRITE" : "READ");

                while (conn.state == STATE_WRITING && conn.bytes_left) {
                    size_t produced =
                        std::min(send_buffer.size(), conn.bytes_left);
                    ssize_t send_or_err =
                        m_driver->sendto(m_fd,
                                         send_buffer.data(),
                                         produced,
                                         0,
                                         &conn.client,
                                         get_sa_len(&conn.client));
                    if (send_or_err == -1) {
                        char ntopbuf[INET6_ADDRSTRLEN];
                        const char* addr = inet_ntop(conn.client.sa_family,
                                                     get_sa_addr(&conn.client),
                                                     ntopbuf,
                                                     INET6_ADDRSTRLEN);
                        OP_LOG(OP_LOG_ERROR,
                               "Error sending to %s:%d: %s\n",
                               addr ? addr : "unknown",
                               ntohs(get_sa_port(&conn.client)),
                               strerror(errno));
                        conn.state = STATE_ERROR;
                        conn.bytes_left = 0;
                        m_stat.errors++;
                    } else {
                        conn.bytes_left -= send_or_err;
                        m_stat.bytes_sent += send_or_err;
                        OP_LOG(
                            OP_LOG_INFO, "--- SERVER SENT %zd\n", send_or_err);
                    }
                }
                m_stat.closed++;
            }
        }
    });
}

} // namespace openperf::network::internal::firehose
