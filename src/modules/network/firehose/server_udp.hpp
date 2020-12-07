#ifndef _OP_NETWORK_FIREHOSE_SERVER_UDP_HPP_
#define _OP_NETWORK_FIREHOSE_SERVER_UDP_HPP_

#include <atomic>
#include <thread>
#include <vector>

#include "server.hpp"

namespace openperf::network::internal::firehose {

class server_udp final : public server
{
private:
    std::atomic_bool m_stopped;
    std::thread m_worker_thread;

    int udp_write(connection_t&);
    tl::expected<int, std::string> new_server(int domain, in_port_t port);
    void run_worker_thread();

public:
    server_udp(in_port_t port, const drivers::driver_ptr& driver);
    server_udp(const server_udp&) = delete;
    ~server_udp() override;
};

} // namespace openperf::network::internal::firehose

#endif
