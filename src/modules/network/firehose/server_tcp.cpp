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
#include "core/op_event_loop.hpp"
#include "utils/random.hpp"
#include "framework/utils/memcpy.hpp"

#include "protocol.hpp"

namespace openperf::network::internal::firehose {

const static size_t tcp_backlog = 128;
const std::string accept_endpoint = "inproc://firehose_tcp_server_accept";
const std::string ctrl_endpoint = "inproc://firehose_tcp_server_ctrl";

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

static inline bool sock_is_readable(void* sock)
{
    uint32_t flags = 0;
    size_t flag_size = sizeof(flags);
    if (zmq_getsockopt(sock, ZMQ_EVENTS, &flags, &flag_size) != 0) return false;
    return (flags & ZMQ_POLLIN);
}

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
        op_socket_get_server(context, ZMQ_PUSH, accept_endpoint.data()));
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
                if (zmq_send(m_accept_pub.get(), &conn, sizeof(conn), 0) < 0) {
                    OP_LOG(OP_LOG_ERROR,
                           "tcp_acceptor failed to send fd=%d\n",
                           conn.fd);
                    m_driver->close(conn.fd);
                }
            }
        }
    }
}

class tcp_connection_reading_state
{
public:
    static int on_read(const struct op_event_data* data, void* arg)
    {
        auto con = reinterpret_cast<tcp_connection_t*>(arg);
        if (!con || !con->worker) return -1;
        return con->worker->do_read(*con);
    }
    static int on_delete(const struct op_event_data* data, void* arg)
    {
        auto con = reinterpret_cast<tcp_connection_t*>(arg);
        if (!con || !con->worker) return -1;
        return con->worker->do_close(*con);
    }
    static constexpr op_event_callbacks callbacks = {
        .on_read = tcp_connection_reading_state::on_read,
        .on_delete = tcp_connection_reading_state::on_delete};
};

class tcp_connection_writing_state
{
public:
    static int on_write(const struct op_event_data* data, void* arg)
    {
        auto con = reinterpret_cast<tcp_connection_t*>(arg);
        if (!con || !con->worker) return -1;
        return con->worker->do_write(*con);
    }
    static int on_delete(const struct op_event_data* data, void* arg)
    {
        auto con = reinterpret_cast<tcp_connection_t*>(arg);
        if (!con || !con->worker) return -1;
        return con->worker->do_close(*con);
    }
    static constexpr op_event_callbacks callbacks = {
        .on_write = tcp_connection_writing_state::on_write,
        .on_delete = tcp_connection_writing_state::on_delete};
};

tcp_worker::tcp_worker(const drivers::driver_ptr& driver,
                       void* context,
                       server::stat_t& stat)
    : m_driver(driver)
    , m_stat(stat)
    , m_loop(new core::event_loop())
    , m_running(false)
    , m_read_buffer(recv_buffer_size)
    , m_write_buffer(send_buffer_size)
{
    // ZMQ sockets are edge triggered so need to use edge triggered mode
    m_loop->set_edge_triggered(true);

    m_ctrl_sub.reset(
        op_socket_get_client_subscription(context, ctrl_endpoint.data(), ""));
    if (!m_ctrl_sub) {
        constexpr auto err_msg =
            "Network load generator tcp worker failed to create control socket";
        OP_LOG(OP_LOG_ERROR, err_msg);
        throw std::runtime_error(err_msg);
    }
    m_accept_sub.reset(
        op_socket_get_client(context, ZMQ_PULL, accept_endpoint.data()));
    if (!m_accept_sub) {
        constexpr auto err_msg =
            "Network load generator tcp worker failed to create accept socket";
        OP_LOG(OP_LOG_ERROR, err_msg);
        throw std::runtime_error(err_msg);
    }

    utils::op_prbs23_fill(m_write_buffer.data(), m_write_buffer.size());
}

tcp_worker::~tcp_worker() {}

void tcp_worker::start()
{
    m_running = true;
    m_thread = std::thread([this] {
        op_thread_setname("op_net_srv_w");
        this->run();
        op_log_close();
    });
}

void tcp_worker::join()
{
    if (m_thread.joinable()) { m_thread.join(); }
}

void tcp_worker::run()
{
    struct op_event_callbacks ctrl_callbacks = {
        .on_read = [](const op_event_data* data, void* arg) -> int {
            auto w = reinterpret_cast<tcp_worker*>(arg);
            if (!w) return -1;
            return w->do_control();
        }};
    m_loop->add(m_ctrl_sub.get(), &ctrl_callbacks, this);

    struct op_event_callbacks accept_callbacks = {
        .on_read = [](const op_event_data* data, void* arg) -> int {
            auto w = reinterpret_cast<tcp_worker*>(arg);
            if (!w) return -1;
            return w->do_accept();
        }};
    m_loop->add(m_accept_sub.get(), &accept_callbacks, this);

    if (sock_is_readable(m_ctrl_sub.get())) { do_control(); }
    if (sock_is_readable(m_accept_sub.get())) { do_accept(); }

    while (m_running) { m_loop->run(); }

    // Close all remaining connections
    m_loop->purge();
    assert(m_connections.empty());
}

int tcp_worker::do_accept()
{
    connection_msg_t msg;

    while (zmq_recv(m_accept_sub.get(), &msg, sizeof(msg), ZMQ_DONTWAIT) >= 0) {
        auto r = network_sockaddr_assign(&msg.client);
        if (!r) {
            OP_LOG(OP_LOG_ERROR, "Error getting TCP connection sockaddr");
            m_driver->close(msg.fd);
            continue;
        }
        auto conn = std::unique_ptr<tcp_connection_t>(
            new tcp_connection_t{.fd = msg.fd,
                                 .state = STATE_WAITING,
                                 .client = r.value(),
                                 .worker = this});
        m_loop->add(
            conn->fd, &tcp_connection_reading_state::callbacks, conn.get());
        m_connections[msg.fd] = std::move(conn);
        m_stat.connections++;
    }

    if (errno != EAGAIN) { return -1; }
    return 0;
}

int tcp_worker::do_control()
{
    uint32_t msg = 0;

    while (zmq_recv(m_ctrl_sub.get(), &msg, sizeof(msg), ZMQ_DONTWAIT) >= 0) {
        m_running = false;
        m_loop->exit();
    }

    if (errno != EAGAIN) { return -1; }
    return 0;
}

int tcp_worker::do_close(tcp_connection_t& conn)
{
    if (conn.state == STATE_ERROR) m_stat.errors++;
    m_stat.closed++;
    m_driver->close(conn.fd);
    m_connections.erase(conn.fd);

    return 0;
}

int tcp_worker::do_read(tcp_connection_t& conn)
{
    uint8_t* recv_cursor;
    ssize_t recv_or_err = 0;

    if (conn.state == STATE_WRITING) return 0;

    do {
        recv_cursor = m_read_buffer.data();
        if (conn.request.size()) {
            /* handle leftovers from last go round */
            utils::memcpy(
                m_read_buffer.data(), conn.request.data(), conn.request.size());
            recv_cursor += conn.request.size();
        }

        if ((recv_or_err =
                 m_driver->recv(conn.fd,
                                recv_cursor,
                                m_read_buffer.size() - conn.request.size(),
                                0))
            == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Process remaining data if there is enough available */
                if (conn.request.size() < firehose::net_request_size) {
                    return 0;
                }
                recv_or_err = 0;
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
                return -1;
            }
        } else if (recv_or_err == 0) {
            conn.state = STATE_DONE;
            return -1;
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
                auto req = firehose::parse_request(recv_cursor, bytes_left);
                if (!req) {
                    char ntopbuf[INET6_ADDRSTRLEN];
                    const char* addr =
                        inet_ntop(network_sockaddr_family(conn.client),
                                  network_sockaddr_addr(conn.client),
                                  ntopbuf,
                                  INET6_ADDRSTRLEN);
                    OP_LOG(OP_LOG_ERROR,
                           "Invalid firehose request received "
                           "from %s:%d with %zu bytes\n",
                           addr ? addr : "unknown",
                           ntohs(network_sockaddr_port(conn.client)),
                           bytes_left);
                    conn.state = STATE_ERROR;
                    return -1;
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
                size_t consumed = std::min(bytes_left, conn.bytes_left);
                conn.bytes_left -= consumed;
                bytes_left -= consumed;
                recv_cursor += consumed;

                if (conn.bytes_left == 0) { conn.state = STATE_WAITING; }

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
                return -1;
            }
            }
        }
    } while (recv_or_err == recv_buffer_size);

    if (conn.state == STATE_WRITING) { return do_write(conn); }

    return 0;
}

int tcp_worker::do_write(tcp_connection_t& conn)
{
    int flags = 0;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    if (conn.state != STATE_WRITING) { return 0; }

    assert(conn.bytes_left);

    while (conn.bytes_left) {
        size_t to_send = std::min(send_buffer_size, conn.bytes_left);
        ssize_t send_or_err =
            m_driver->send(conn.fd, m_write_buffer.data(), to_send, flags);
        if (send_or_err == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                m_loop->update(conn.fd,
                               &tcp_connection_writing_state::callbacks);
                return 0;
            }
            int saved_errno = errno;
            char ntopbuf[INET6_ADDRSTRLEN];
            const char* addr = inet_ntop(network_sockaddr_family(conn.client),
                                         network_sockaddr_addr(conn.client),
                                         ntopbuf,
                                         INET6_ADDRSTRLEN);
            OP_LOG(OP_LOG_WARNING,
                   "Error sending to %s:%d: %s",
                   addr ? addr : "unknown",
                   ntohs(network_sockaddr_port(conn.client)),
                   strerror(saved_errno));
            conn.state = STATE_ERROR;
            return -1;
        }

        m_stat.bytes_sent += send_or_err;
        conn.bytes_left -= send_or_err;
    }

    conn.state = STATE_WAITING;
    m_loop->update(conn.fd, &tcp_connection_reading_state::callbacks);

    return 0;
}

server_tcp::server_tcp(in_port_t port,
                       const std::optional<std::string>& interface,
                       const drivers::driver_ptr& driver)
    : m_context(zmq_ctx_new())
{
    // Set ZMQ threads to 0 for context
    // Note: Value 0 valid only for inproc:// transport
    if (zmq_ctx_set(m_context.get(), ZMQ_IO_THREADS, 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ set IO threads to 0: %s",
               zmq_strerror(errno));
    }

    m_driver = driver;

    m_ctrl_pub.reset(
        op_socket_get_server(m_context.get(), ZMQ_PUB, ctrl_endpoint.data()));
    if (!m_ctrl_pub) {
        constexpr auto err_msg =
            "Network TCP load server failed to create control socket";
        OP_LOG(OP_LOG_ERROR, err_msg);
        throw std::runtime_error(err_msg);
    }

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
        m_acceptor = std::make_unique<tcp_acceptor>(driver, m_context.get());
        m_acceptor->start(acceptfd);
    } catch (...) {
        m_driver->close(acceptfd);
        throw;
    }

    start_workers();
}

server_tcp::~server_tcp()
{
    m_acceptor->stop();
    m_acceptor.reset();

    stop_workers();

    m_ctrl_pub.reset();
}

void server_tcp::start_workers()
{
    // Accept thread has to be started first to avoid ZMQ fault
    assert(m_acceptor->running());

    m_workers.push_back(
        std::make_unique<tcp_worker>(m_driver, m_context.get(), m_stat));
    m_workers.back()->start();
}

void server_tcp::stop_workers()
{
    if (m_workers.empty()) return;

    // Send stop message to all workers
    uint32_t ctrl_msg = 1;
    if (zmq_send(m_ctrl_pub.get(), &ctrl_msg, sizeof(ctrl_msg), 0) < 0) {
        OP_LOG(OP_LOG_ERROR,
               "Network TCP load server failed to send stop message.  %s",
               zmq_strerror(errno));
    }

    // Wait for all workers to exit
    for (auto& worker : m_workers) { worker->join(); }

    m_workers.clear();
}

} // namespace openperf::network::internal::firehose
