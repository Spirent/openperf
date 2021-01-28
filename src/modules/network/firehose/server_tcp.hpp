#ifndef _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_

#include <atomic>
#include <thread>
#include <vector>
#include <zmq.h>

#include "server.hpp"
#include "core/op_socket.h"

namespace openperf::core {
class event_loop;
}

namespace openperf::network::internal::firehose {

class tcp_acceptor
{
private:
    drivers::driver_ptr m_driver;
    std::unique_ptr<void, op_socket_deleter> m_accept_pub;
    std::thread m_thread;
    int m_eventfd;
    int m_acceptfd;
    bool m_running;

    void run();

public:
    tcp_acceptor(const drivers::driver_ptr& driver, void* context);
    ~tcp_acceptor();

    void start(int acceptfd);
    void stop();
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

    std::atomic_bool m_stopped;
    std::vector<std::unique_ptr<std::thread>> m_worker_threads;
    std::unique_ptr<void, zmq_ctx_deleter> m_context;
    tcp_acceptor m_acceptor;

    int tcp_write(connection_t&, std::vector<uint8_t> send_buffer);
    void run_accept_thread();
    void run_worker_thread();

public:
    server_tcp(in_port_t port,
               const std::optional<std::string>& interface,
               const drivers::driver_ptr& driver);
    server_tcp(const server_tcp&) = delete;
    ~server_tcp() override;
};

} // namespace openperf::network::internal::firehose

#endif
