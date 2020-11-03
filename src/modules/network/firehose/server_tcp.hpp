#ifndef _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_

#include <atomic>
#include <thread>
#include <vector>
#include <zmq.h>
#include <netinet/in.h>

#include "server_common.hpp"

namespace openperf::network::internal::firehose {

class server_tcp : public server
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

    std::vector<uint8_t> m_send_buffer;
    std::atomic_bool m_stopped;
    std::thread m_accept_thread;
    std::vector<std::unique_ptr<std::thread>> m_worker_threads;
    std::unique_ptr<void, zmq_ctx_deleter> m_context;

    int tcp_write(connection_t&);

public:
    server_tcp(in_port_t port);
    server_tcp(const server_tcp&) = delete;
    ~server_tcp() override;

    void run_accept_thread() override;
    void run_worker_thread() override;
};

} // namespace openperf::network::internal::firehose

#endif