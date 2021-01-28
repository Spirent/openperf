#include "server_tcp.hpp"

#include <cassert>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <netinet/tcp.h>

#include "core/op_log.h"
#include "core/op_thread.h"
#include "core/op_socket.h"
#include "utils/random.hpp"
#include "framework/utils/memcpy.hpp"

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

tl::expected<int, std::string>
create_server_socket(drivers::driver* drv,
                     int domain,
                     in_port_t port,
                     const std::optional<std::string>& interface)
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
    if ((sock = drv->socket(domain, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to open %s TCP server socket: %s\n",
               (domain == AF_INET6) ? "IPv6" : "IPv4",
               strerror(errno));
        return tl::make_unexpected<std::string>(strerror(errno));
    }

    if (interface) {
        if (drv->setsockopt(sock,
                            SOL_SOCKET,
                            SO_BINDTODEVICE,
                            interface.value().c_str(),
                            interface.value().size())
            < 0) {
            auto err = errno;
            drv->close(sock);
            return tl::make_unexpected<std::string>(strerror(err));
        }
    }

    /* Update to non-blocking socket */
    int flags = drv->fcntl(sock, F_GETFL);
    if (flags == -1) {
        auto err = errno;
        drv->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (drv->fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        auto err = errno;
        drv->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (drv->setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))
        != 0) {
        auto err = errno;
        drv->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (drv->setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable))
        != 0) {
        auto err = errno;
        drv->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (drv->bind(sock, server_ptr, get_sa_len(server_ptr)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to bind to socket (domain = %d, protocol = TCP): %s\n",
               domain,
               strerror(errno));
        auto err = errno;
        drv->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    if (drv->listen(sock, tcp_backlog) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to listen on socket %d: %s\n",
               sock,
               strerror(errno));
        auto err = errno;
        drv->close(sock);
        return tl::make_unexpected<std::string>(strerror(err));
    }

    return sock;
}

tcp_acceptor::tcp_acceptor(const drivers::driver_ptr& driver, void* context)
    : m_driver(driver)
    , m_eventfd(-1)
    , m_acceptfd(-1)
    , m_running(false)
{
    m_accept_pub.reset(
        op_socket_get_server(context, ZMQ_PUSH, endpoint.data()));
    if (!m_accept_pub) {
        throw std::runtime_error("Unable to create ZMQ push socket");
    }

    m_eventfd = eventfd(0, EFD_NONBLOCK);
    if (m_eventfd < 0) {
        throw std::system_error(
            errno, std::generic_category(), "Unable to create eventfd");
    }
}

tcp_acceptor::~tcp_acceptor()
{
    if (m_running) {
        try {
            stop();
        } catch (...) {
            // no exceptions in destructor
        }
    }

    close(m_eventfd);
}

void tcp_acceptor::start(int acceptfd)
{
    if (m_running) throw std::runtime_error("tcp_acceptor is already running");

    m_running = true;
    m_acceptfd = acceptfd;
    m_thread = std::thread([this] {
        op_thread_setname("op_net_srv_acc");
        this->run();
        op_log_close();
    });
}

void tcp_acceptor::stop()
{
    if (!m_running) return;

    // signal thread to stop
    m_running = false;

    // wakeup the thread
    if (eventfd_write(m_eventfd, 1) < 0) {
        OP_LOG(OP_LOG_ERROR, "Failed writing to tcp_acceptor eventfd");
        throw std::system_error(
            errno, std::generic_category(), "Could not write to eventfd");
    }

    // wait for thread to stop
    m_thread.join();
    m_driver->shutdown(m_acceptfd, SHUT_RDWR);
    m_driver->close(m_acceptfd);
    m_acceptfd = -1;
}

void tcp_acceptor::run()
{
    while (m_running) {
        std::array<struct pollfd, 2> pfd;
        pfd[0] = {.fd = m_eventfd, .events = POLLIN, .revents = 0};
        pfd[1] = {.fd = m_acceptfd, .events = POLLIN, .revents = 0};
        int npoll = poll(pfd.data(), pfd.size(), -1);
        if (npoll < 0 && errno != EINTR) {
            OP_LOG(OP_LOG_ERROR, "poll failed.  %s", strerror(errno));
            break;
        }
        if (npoll <= 0) { continue; }

        if (pfd[0].revents & POLLIN) {
            eventfd_t value = 0;
            eventfd_read(m_eventfd, &value);
        }
        if (pfd[1].revents & POLLIN) {
            sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            auto saddr = reinterpret_cast<sockaddr*>(&addr);
            int conn_fd;

            while ((conn_fd = m_driver->accept(
                        m_acceptfd, saddr, &addr_len, SOCK_NONBLOCK))
                   != -1) {
                int enable = 1;
                if (m_driver->setsockopt(conn_fd,
                                         IPPROTO_TCP,
                                         TCP_NODELAY,
                                         &enable,
                                         sizeof(enable))
                    != 0) {
                    OP_LOG(OP_LOG_ERROR,
                           "Failed to enable TCP_NODELAY option.   %s",
                           strerror(errno));
                    m_driver->close(conn_fd);
                    continue;
                }
                auto conn = connection_msg_t{.fd = conn_fd};
                memcpy(&conn.client, saddr, get_sa_len(saddr));
                zmq_send(m_accept_pub.get(), &conn, sizeof(conn), ZMQ_DONTWAIT);
            }
        }
    }
}

server_tcp::server_tcp(in_port_t port,
                       const std::optional<std::string>& interface,
                       const drivers::driver_ptr& driver)
    : m_stopped(false)
    , m_context(zmq_ctx_new())
    , m_acceptor(driver, m_context.get())
{
    // Set ZMQ threads to 0 for context
    // Note: Value 0 valid only for inproc:// transport
    if (zmq_ctx_set(m_context.get(), ZMQ_IO_THREADS, 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ set IO threads to 0: %s",
               zmq_strerror(errno));
    }

    m_driver = driver;

    /* IPv6 any supports IPv4 and IPv6 */
    tl::expected<int, std::string> res;
    if ((res = create_server_socket(m_driver.get(), AF_INET6, port, interface));
        res) {
        OP_LOG(OP_LOG_DEBUG, "Network TCP load server IPv4/IPv6.\n");
    } else if ((res = create_server_socket(
                    m_driver.get(), AF_INET, port, interface));
               res) {
        OP_LOG(OP_LOG_DEBUG, "Network TCP load server IPv4.\n");
    } else {
        throw std::runtime_error("Cannot create TCP server: " + res.error());
    }

    int acceptfd = res.value();
    try {
        m_acceptor.start(acceptfd);
    } catch (...) {
        m_driver->close(acceptfd);
        throw;
    }

    run_worker_thread();
}

server_tcp::~server_tcp()
{
    m_stopped.store(true, std::memory_order_relaxed);
    m_acceptor.stop();

    zmq_ctx_shutdown(m_context.get());

    for (auto& thread : m_worker_threads) {
        if (thread->joinable()) thread->join();
    }
}

int server_tcp::tcp_write(connection_t& conn, std::vector<uint8_t> send_buffer)
{

    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    if (conn.state != STATE_WRITING) { return 0; }

    assert(conn.bytes_left);

    errno = 0;
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

    if (conn.bytes_left == 0) { conn.state = STATE_WAITING; }

    return 0;
}

void server_tcp::run_worker_thread()
{
    // Accept thread has to be started first to avoid ZMQ fault
    assert(m_acceptor.running());

    void* sync =
        op_socket_get_client(m_context.get(), ZMQ_PULL, endpoint.data());
    m_worker_threads.push_back(std::make_unique<std::thread>([this, sync] {
        // Set the thread name
        op_thread_setname("op_net_srv_w");

        std::vector<connection_t> connections;
        connection_msg_t conn_buffer;
        std::vector<uint8_t> send_buffer(send_buffer_size);
        utils::op_prbs23_fill(send_buffer.data(), send_buffer.size());
        std::vector<uint8_t> recv_buffer(recv_buffer_size);

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
                auto r = network_sockaddr_assign(&conn_buffer.client);
                if (!r) {
                    OP_LOG(OP_LOG_ERROR, "Error getting sockaddr");
                    m_driver->close(conn.fd);
                    continue;
                }
                conn.client = r.value();
                connections.push_back(std::move(conn));
                m_stat.connections++;
            }

            for (auto& conn : connections) {
                uint8_t* recv_cursor;

                ssize_t recv_or_err = 0;
                do {
                    recv_cursor = recv_buffer.data();
                    if (conn.request.size()) {
                        /* handle leftovers from last go round */
                        utils::memcpy(recv_buffer.data(),
                                      conn.request.data(),
                                      conn.request.size());
                        recv_cursor += conn.request.size();
                    }

                    if ((recv_or_err = m_driver->recv(conn.fd,
                                                      recv_cursor,
                                                      recv_buffer.size()
                                                          - conn.request.size(),
                                                      0))
                        == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            if (conn.state == STATE_WRITING) {
                                recv_or_err = 0;
                            } else {
                                break;
                            }
                        } else {
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
                        }
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
                                    "from %s:%d with %zu bytes\n",
                                    addr ? addr : "unknown",
                                    ntohs(network_sockaddr_port(conn.client)),
                                    bytes_left);
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
                            bytes_left = 0;
                            break;
                        }
                        }
                    }
                    if (conn.state == STATE_WRITING) {
                        tcp_write(conn, send_buffer);
                        break;
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
                                                     m_driver->close(conn.fd);
                                                     return true;
                                                 }
                                                 return false;
                                             }),
                              connections.end());
        }

        // Close all remaining connections
        for (auto& conn : connections) {
            if (conn.state == STATE_ERROR) m_stat.errors++;
            m_stat.closed++;
            m_driver->close(conn.fd);
        }

        op_socket_close(sync);
    }));
}

} // namespace openperf::network::internal::firehose
