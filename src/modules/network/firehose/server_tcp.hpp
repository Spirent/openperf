#ifndef _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_TCP_HPP_

#include <atomic>
#include <thread>
#include <vector>
#include <netinet/in.h>

#include "server_common.hpp"

namespace openperf::network::internal {

class firehose_server_tcp : public firehose_server
{
public:
    firehose_server_tcp(in_port_t port);
    firehose_server_tcp(const firehose_server_tcp&) = delete;
    ~firehose_server_tcp() override;

    void run_accept_thread() override;
    void run_worker_thread() override;

private:
    int tcp_write(connection_t&);

    std::vector<uint8_t> m_send_buffer;
    std::atomic_bool m_stopped;
    std::thread m_accept_thread, m_worker_thread;
    std::vector<connection_t> m_connections;
};

} // namespace openperf::network::internal

#endif