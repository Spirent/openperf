#ifndef _OP_NETWORK_FIREHOSE_SERVER_UDP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_UDP_HPP_

#include <atomic>
#include <thread>
#include <vector>
#include <zmq.h>

#include "server_common.hpp"

namespace openperf::network::internal::firehose {

class server_udp : public server
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
    std::thread m_worker_thread;

    int udp_write(connection_t&);
    int new_server(int domain, in_port_t port);
    void run_worker_thread();

public:
    server_udp(in_port_t port, const drivers::network_driver_ptr& driver);
    server_udp(const server_udp&) = delete;
    ~server_udp() override;
};

} // namespace openperf::network::internal::firehose

#endif