#ifndef _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_

#include <atomic>
#include <thread>
#include <vector>
#include <map>
#include <zmq.h>

#include "server.hpp"
#include "core/op_socket.h"

namespace openperf::core {
class event_loop;
}

namespace openperf::network::internal::firehose {

class tcp_worker;

struct tcp_connection_t
{
    int fd;
    connection_state_t state;
    size_t bytes_left;
    std::vector<uint8_t> request;
    network_sockaddr client;
    tcp_worker* worker;
};

class tcp_acceptor
{
private:
    drivers::driver_ptr m_driver;
    std::unique_ptr<void, op_socket_deleter> m_accept_pub;
    std::thread m_thread;
    int m_eventfd;  /* eventfd used to wakeup from poll */
    int m_acceptfd; /* socket fd used for accept() */
    bool m_running; /* acceptor is running */

    void run();

public:
    tcp_acceptor(const drivers::driver_ptr& driver, void* context);
    tcp_acceptor(const tcp_acceptor&) = delete;
    tcp_acceptor(tcp_acceptor&&) = delete;
    tcp_acceptor& operator=(const tcp_acceptor&) = delete;
    tcp_acceptor& operator=(tcp_acceptor&&) = delete;
    ~tcp_acceptor();

    void start(int acceptfd);
    void stop();
    bool running() const { return m_running; }
};

class tcp_worker
{
private:
    friend class tcp_connection_reading_state;
    friend class tcp_connection_writing_state;

    drivers::driver_ptr m_driver;
    server::stat_t& m_stat;
    std::unique_ptr<core::event_loop> m_loop;
    std::unique_ptr<void, op_socket_deleter> m_ctrl_sub;
    std::unique_ptr<void, op_socket_deleter> m_accept_sub;
    std::thread m_thread;
    bool m_running;
    std::map<int, std::unique_ptr<tcp_connection_t>> m_connections;
    std::vector<uint8_t> m_read_buffer;
    std::vector<uint8_t> m_write_buffer;

    void run();

    int do_accept();
    int do_control();
    int do_close(tcp_connection_t& conn);
    int do_read(tcp_connection_t& conn);
    int do_write(tcp_connection_t& conn);

public:
    tcp_worker(const drivers::driver_ptr& driver,
               void* context,
               server::stat_t& stat);
    tcp_worker(const tcp_worker&) = delete;
    tcp_worker(tcp_worker&&) = delete;
    tcp_worker& operator=(const tcp_worker&) = delete;
    tcp_worker& operator=(tcp_worker&&) = delete;
    ~tcp_worker() = default;

    void start();
    void join();
    bool running() const { return m_running; }
};

class server_tcp final : public server
{
private:
    struct zmq_ctx_deleter
    {
        void operator()(void* ctx) const
        {
            zmq_ctx_shutdown(ctx);
            zmq_ctx_term(ctx);
        }
    };

    std::unique_ptr<void, zmq_ctx_deleter> m_context;
    std::unique_ptr<void, op_socket_deleter> m_ctrl_pub;
    std::unique_ptr<tcp_acceptor> m_acceptor;
    std::vector<std::unique_ptr<tcp_worker>> m_workers;

    void start_workers();
    void stop_workers();

public:
    server_tcp(in_port_t port,
               const std::optional<std::string>& interface,
               const drivers::driver_ptr& driver);
    server_tcp(const server_tcp&) = delete;
    ~server_tcp() override;
};

} // namespace openperf::network::internal::firehose

#endif
