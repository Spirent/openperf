#include <thread>

#include "core/op_core.h"
#include "socket/server/api_server.h"

namespace openperf::socket::server {

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

int op_socket_server_init(void* context, void* state)
{
    auto s = reinterpret_cast<openperf::socket::server::service*>(state);
    s->init(context);
    return (0);
}

int op_socket_server_start(void *state)
{
    auto s = reinterpret_cast<openperf::socket::server::service*>(state);
    return (s->start());
}

void op_socket_server_stop(void *state)
{
    auto s = reinterpret_cast<openperf::socket::server::service*>(state);
    s->stop();
    delete s;
}

REGISTER_MODULE(socket_server,
                INIT_MODULE_INFO("socket-server",
                                 "Core module providing access to the TCP/IP stack via libopenperf-shim.so",
                                 openperf::socket::server::module_version),
                new openperf::socket::server::service(),
                nullptr,
                op_socket_server_init,
                nullptr,
                op_socket_server_start,
                op_socket_server_stop);

}
