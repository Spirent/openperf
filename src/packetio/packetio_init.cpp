#include <cstring>
#include <memory>
#include <thread>

#include <zmq.h>

#include "packetio/packetio_port_api.h"
#include "core/icp_core.h"

namespace icp {
namespace packetio {

struct service {
    ~service() {
        if (worker.joinable()) {
            worker.join();
        }
    }

    void init(void *context) {
        loop.reset(new icp::core::event_loop());
        port_server.reset(new port::api::server(context, *loop));
    }

    void start()
    {
        worker = std::thread([this]() {
                                 icp_thread_setname("icp_packetio");
                                 loop->run();
                             });
    }

    std::unique_ptr<icp::core::event_loop> loop;
    std::unique_ptr<port::api::server> port_server;
    std::thread worker;
};

}
}
extern "C" {

int packetio_init(void *context, void *state)
{
    icp::packetio::service *s = reinterpret_cast<icp::packetio::service *>(state);
    s->init(context);
    s->start();
    return (0);
}

struct icp_module packetio = {
    .name = "packetio",
    .state = new icp::packetio::service(),
    .init = packetio_init,
};

REGISTER_MODULE(packetio)

}
