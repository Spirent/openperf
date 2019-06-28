#include <thread>

#include "core/icp_core.h"
#include "socket/server/api_server.h"

namespace icp::socket::server {

static constexpr int module_version = 1;

struct service {
    void init(void* context) {
        m_server = std::make_unique<api::server>(context);
    }

    int start() {
        return (m_server->start());
    }

    void stop() {
        m_server->stop();
    }

    std::unique_ptr<api::server> m_server;
};

}

extern "C" {

int icp_socket_server_init(void* context, void* state)
{
    auto s = reinterpret_cast<icp::socket::server::service*>(state);
    s->init(context);
    return (0);
}

int icp_socket_server_start(void *state)
{
    auto s = reinterpret_cast<icp::socket::server::service*>(state);
    return (s->start());
}

void icp_socket_server_stop(void *state)
{
    auto s = reinterpret_cast<icp::socket::server::service*>(state);
    s->stop();
    delete s;
}

REGISTER_MODULE(socket_server,
                INIT_MODULE_INFO("socket-server",
                                 "Core module providing access to the TCP/IP stack via libicp-shim.so",
                                 icp::socket::server::module_version),
                new icp::socket::server::service(),
                nullptr,
                icp_socket_server_init,
                nullptr,
                icp_socket_server_start,
                icp_socket_server_stop);

}
