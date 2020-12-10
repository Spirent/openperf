#include "server_tcp.hpp"

#include <cassert>
#include <string>
#include <fcntl.h>

#include <netinet/tcp.h>
#include "core/op_log.h"
#include "core/op_thread.h"
#include "core/op_socket.h"
#include "utils/random.hpp"

#include "protocol.hpp"

namespace openperf::network::internal::firehose {

const static size_t tcp_backlog = 128;
const std::string endpoint = "inproc://firehose_server";

struct connection_msg_t
{
    int fd;
    union
    {
        struct sockaddr client;
        struct sockaddr_in client4;
        struct sockaddr_in6 client6;
    };
};

tl::expected<int, std::string> server_tcp::new_server(
    int domain, in_port_t port, std::optional<std::string> interface)
{
    struct sockaddr_storage client_storage;
    auto server_ptr = reinterpret_cast<sockaddr*>(&client_storage);
    std::string domain_str;

    switch (domain) {
    case AF_INET: {
        auto server4 = reinterpret_cast<sockaddr_in*>(server_ptr);
        server4->sin_family = AF_INET;
        server4->sin_port = htons(port);
        server4->sin_addr.s_addr = htonl(INADDR_ANY);
        domain_str = "IPv4";
        break;
    }
    case AF_INET6: {
        auto server6 = reinterpret_cast<sockaddr_in6*>(server_ptr);
        server6->sin6_family = AF_INET6;
        server6->sin6_port = htons(port);
        server6->sin6_addr = in6addr_any;
        domain_str = "IPv6";
        break;
    }
    default:
        return tl::make_unexpected<std::string>(strerror(EINVAL));
    }

    int sock = 0, enable = true;
    if ((sock = m_driver->socket(domain, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        OP_LOG(OP_LOG_WARNING,
               "Unable to open %s TCP server socket: %s\n",
               domain_str.c_str(),
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

    if (m_driver->setsockopt(
            sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable))
        != 0) {
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (m_driver->bind(sock, server_ptr, get_sa_len(server_ptr)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to bind to socket (domain = %d, protocol = TCP): %s\n",
               domain,
               strerror(errno));
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (m_driver->listen(sock, tcp_backlog) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to listen on socket %d: %s\n",
               sock,
               strerror(errno));
        auto err = errno;
        m_driver->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    return sock;
}

server_tcp::server_tcp(in_port_t port,
                       std::optional<std::string> interface,
                       std::optional<int> domain,
                       const drivers::driver_ptr& driver)
    : m_stopped(false)
    , m_context(zmq_ctx_new())
{
    // Set ZMQ threads to 0 for context
    // Note: Value 0 valid only for inproc:// transport
    if (zmq_ctx_set(m_context.get(), ZMQ_IO_THREADS, 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ set IO threads to 0: %s",
               zmq_strerror(errno));
    }

    m_driver = driver;
    tl::expected<int, std::string> res;
    if (domain) {
        if ((res = new_server(domain.value(), port, interface)); res) {
            OP_LOG(OP_LOG_INFO,
                   "Network TCP load server %s.\n",
                   (domain.value() == AF_INET6) ? "IPv6" : "IPv4");
        } else {
            throw std::runtime_error("Cannot create TCP server: "
                                     + res.error());
        }
    } else {
        if ((res = new_server(AF_INET6, port, interface)); res) {
            OP_LOG(OP_LOG_INFO, "Network TCP load server IPv4/IPv6.\n");
        } else if ((res = new_server(AF_INET, port, interface)); res) {
            OP_LOG(OP_LOG_INFO, "Network TCP load server IPv4.\n");
        } else {
            throw std::runtime_error("Cannot create TCP server: "
                                     + res.error());
        }
    }
    m_fd = res.value();

    run_accept_thread();
    run_worker_thread();
}

server_tcp::~server_tcp()
{
    m_stopped.store(true, std::memory_order_relaxed);
    zmq_ctx_shutdown(m_context.get());
    if (m_fd.load() >= 0) {
        m_driver->shutdown(m_fd.load(), SHUT_RDWR);
        m_driver->close(m_fd);
    }

    if (m_accept_thread.joinable()) m_accept_thread.join();
    for (auto& thread : m_worker_threads) {
        if (thread->joinable()) thread->join();
    }
}

void server_tcp::run_accept_thread()
{
    if (m_accept_thread.joinable()) return;

    m_accept_thread = std::thread([&] {
        // Set the thread name
        op_thread_setname("op_net_srv_acc");

        // Run the loop of the thread
        sockaddr_storage client_storage;
        auto client = reinterpret_cast<sockaddr*>(&client_storage);
        socklen_t client_length = sizeof(client_storage);

        void* sync =
            op_socket_get_server(m_context.get(), ZMQ_PUSH, endpoint.data());

        int connect_fd;
        while ((connect_fd = accept(m_fd.load(), client, &client_length))
               != -1) {
            int enable = 1;
            if (m_driver->setsockopt(connect_fd,
                                     IPPROTO_TCP,
                                     TCP_NODELAY,
                                     &enable,
                                     sizeof(enable))
                != 0) {
                m_driver->close(connect_fd);
                continue;
            }
            // Change the socket into non-blocking state
            int flags;
            if ((flags = m_driver->fcntl(connect_fd, F_GETFL, 0)); flags >= 0) {
                flags |= O_NONBLOCK;
                if (m_driver->fcntl(connect_fd, F_SETFL, O_NONBLOCK)) {
                    OP_LOG(OP_LOG_WARNING,
                           "Cannot set non blocking socket (%d): %s\n",
                           connect_fd,
                           strerror(errno));
                }
            } else {
                OP_LOG(OP_LOG_WARNING,
                       "Cannot get file descriptor flags (%d): %s\n",
                       connect_fd,
                       strerror(errno));
            }

            auto conn = connection_msg_t{.fd = connect_fd};
            memcpy(&conn.client, client, get_sa_len(client));
            zmq_send(sync, &conn, sizeof(conn), ZMQ_DONTWAIT);
        }
        op_socket_close(sync);
    });
}

int server_tcp::tcp_write(connection_t& conn, std::vector<uint8_t> send_buffer)
{

    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    if (conn.state != STATE_WRITING) { return 0; }

    assert(conn.bytes_left);

    size_t to_send = std::min(send_buffer_size, conn.bytes_left);
    ssize_t send_or_err =
        m_driver->send(conn.fd, send_buffer.data(), to_send, flags);
    if (send_or_err == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }

        conn.state = STATE_ERROR;
        return -1;
    }

    m_stat.bytes_sent += send_or_err;
    conn.bytes_left -= send_or_err;

    conn.state = STATE_WAITING;

    return 0;
}

void server_tcp::run_worker_thread()
{
    m_worker_threads.push_back(std::make_unique<std::thread>([&] {
        // Set the thread name
        op_thread_setname("op_net_srv_w");

        std::vector<connection_t> connections;
        connection_msg_t conn_buffer;
        std::vector<uint8_t> send_buffer(send_buffer_size);
        utils::op_prbs23_fill(send_buffer.data(), send_buffer.size());
        std::vector<uint8_t> recv_buffer(recv_buffer_size);
        void* sync =
            op_socket_get_client(m_context.get(), ZMQ_PULL, endpoint.data());

        while (!m_stopped.load(std::memory_order_relaxed)) {
            // Receive all accepted connections

            while (zmq_recv(sync,
                            &conn_buffer,
                            sizeof(conn_buffer),
                            (connections.size() > 0) ? ZMQ_DONTWAIT : 0)
                   >= 0) {
                auto conn = connection_t{
                    .fd = conn_buffer.fd,
                    .state = STATE_WAITING,
                };
                network_sockaddr_assign(&conn_buffer.client, conn.client);
                connections.push_back(std::move(conn));
                m_stat.connections++;
            }

            for (auto& conn : connections) {
                uint8_t* recv_cursor = recv_buffer.data();

                ssize_t recv_or_err = 0;
                do {
                    if (conn.state == STATE_WRITING) {
                        tcp_write(conn, send_buffer);
                        break;
                    }

                    if (conn.request.size()) {
                        /* handle leftovers from last go round */
                        recv_buffer = conn.request;
                        recv_cursor += conn.request.size();
                    }

                    if ((recv_or_err = m_driver->recv(conn.fd,
                                                      recv_cursor,
                                                      recv_buffer.size()
                                                          - conn.request.size(),
                                                      0))
                        == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }

                        char ntopbuf[INET6_ADDRSTRLEN];
                        const char* addr =
                            inet_ntop(network_sockaddr_family(conn.client),
                                      network_sockaddr_addr(conn.client),
                                      ntopbuf,
                                      INET6_ADDRSTRLEN);
                        OP_LOG(OP_LOG_ERROR,
                               "Error reading from %s:%d: %s\n",
                               addr ? addr : "unknown",
                               ntohs(network_sockaddr_port(conn.client)),
                               strerror(errno));
                        conn.state = STATE_ERROR;
                        break;
                    } else if (recv_or_err == 0) {
                        conn.state = STATE_DONE;
                        break;
                    }
                    m_stat.bytes_received += recv_or_err;

                    if (conn.request.size()) {
                        /* put the cursor back in the right spot */
                        recv_cursor -= conn.request.size();
                        recv_or_err += conn.request.size();
                        conn.request.clear();
                    }

                    size_t bytes_left = recv_or_err;
                    while (bytes_left) {
                        switch (conn.state) {
                        case STATE_WAITING: {
                            if (bytes_left < firehose::net_request_size) {
                                /* need to wait for more header */
                                conn.request.insert(conn.request.end(),
                                                    recv_cursor,
                                                    recv_cursor + bytes_left);
                                bytes_left = 0;
                                continue;
                            }

                            /* Note: this function updates the note,
                             * including the state */
                            auto req = firehose::parse_request(recv_cursor,
                                                               bytes_left);
                            if (!req) {
                                char ntopbuf[INET6_ADDRSTRLEN];
                                const char* addr = inet_ntop(
                                    network_sockaddr_family(conn.client),
                                    network_sockaddr_addr(conn.client),
                                    ntopbuf,
                                    INET6_ADDRSTRLEN);
                                OP_LOG(
                                    OP_LOG_ERROR,
                                    "Invalid firehose request received "
                                    "from %s:%d\n",
                                    addr ? addr : "unknown",
                                    ntohs(network_sockaddr_port(conn.client)));
                                conn.state = STATE_ERROR;
                                bytes_left = 0;
                                break;
                            }
                            conn.state = (req.value().action == action_t::GET)
                                             ? STATE_WRITING
                                             : STATE_READING;
                            conn.bytes_left = req.value().length;
                            bytes_left -= firehose::net_request_size;
                            recv_cursor += firehose::net_request_size;
                            break;
                        }
                        case STATE_READING: {
                            size_t consumed =
                                std::min(bytes_left, conn.bytes_left);
                            conn.bytes_left -= consumed;
                            bytes_left -= consumed;
                            recv_cursor += consumed;

                            if (conn.bytes_left == 0) {
                                conn.state = STATE_WAITING;
                            }

                            break;
                        }
                        default: {
                            char ntopbuf[INET6_ADDRSTRLEN];
                            const char* addr =
                                inet_ntop(network_sockaddr_family(conn.client),
                                          network_sockaddr_addr(conn.client),
                                          ntopbuf,
                                          INET6_ADDRSTRLEN);
                            OP_LOG(OP_LOG_WARNING,
                                   "Connection from %s:%d is in state %s"
                                   " with %zu bytes left to read. Dropping "
                                   "connection.\n",
                                   addr ? addr : "unknown",
                                   ntohs(network_sockaddr_port(conn.client)),
                                   get_state_string(conn.state),
                                   bytes_left);
                            conn.state = STATE_ERROR;
                            break;
                        }
                        }
                    }
                } while (recv_or_err == recv_buffer_size);
            }

            // Remove closed connections
            connections.erase(std::remove_if(connections.begin(),
                                             connections.end(),
                                             [this](const auto& conn) {
                                                 if (conn.state == STATE_ERROR)
                                                     m_stat.errors++;
                                                 if (conn.state == STATE_DONE
                                                     || conn.state
                                                            == STATE_ERROR) {
                                                     m_stat.closed++;
                                                     return true;
                                                 }
                                                 return false;
                                             }),
                              connections.end());
        }
        op_socket_close(sync);
    }));
}

} // namespace openperf::network::internal::firehose
