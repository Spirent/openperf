#ifndef _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_

#include <atomic>
#include <thread>
#include <vector>
#include <zmq.h>

#include "server.hpp"

namespace openperf::network::internal::firehose {

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
    std::thread m_accept_thread;
    std::vector<std::unique_ptr<std::thread>> m_worker_threads;
    void* m_context;

    int tcp_write(connection_t&, std::vector<uint8_t> send_buffer);
    tl::expected<int, std::string> new_server(int domain, in_port_t port);
    void run_accept_thread();
    void run_worker_thread();

public:
    server_tcp(in_port_t port, const drivers::driver_ptr& driver);
    server_tcp(const server_tcp&) = delete;
    ~server_tcp() override;
};

} // namespace openperf::network::internal::firehose

#endif
