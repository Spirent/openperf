#ifndef _OP_SOCKET_API_SERVER_HPP_
#define _OP_SOCKET_API_SERVER_HPP_

#include <memory>
#include <unordered_map>

#include "core/op_core.h"
#include "packetio/internal_client.hpp"
#include "packetio/generic_event_loop.hpp"
#include "socket/shared_segment.hpp"
#include "socket/unix_socket.hpp"
#include "socket/server/allocator.hpp"

namespace openperf::memory::allocator {
class pool;
}

namespace openperf::socket::server {
class api_handler;
}

namespace openperf::socket::api {

class server
{
    static constexpr size_t shm_size = (1024 * 1024 * 1024); /* 1 GB */

    void* m_context;
    unix_socket m_sock;
    openperf::memory::shared_segment m_shm;
    std::unordered_map<pid_t, std::unique_ptr<socket::server::api_handler>>
        m_handlers;
    std::unordered_multimap<int, pid_t> m_pids;
    std::string m_task;

    socket::server::allocator* allocator() const;

    using event_loop = openperf::packetio::event_loop::generic_event_loop;

    int handle_api_accept(event_loop& loop, const std::any&);
    int handle_api_client(event_loop& loop, std::any arg);
    void handle_api_error(std::any arg);

    int do_client_init(event_loop& loop, int fd);
    int do_client_read(pid_t pid, int fd);

public:
    server(void* context);
    ~server(); /* Explicit destructor needed to avoid pulling in api_handler
                  header */

    int start();
    void stop();
};

} // namespace openperf::socket::api

#endif /* _OP_SOCKET_API_SERVER_HPP_ */
