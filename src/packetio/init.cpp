#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <zmq.h>

#include "packetio/api_server.h"
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
        api::server::make_all(servers, context, *loop);
    }

    void start()
    {
        worker = std::thread([this]() {
                                 icp_thread_setname("icp_packetio");
                                 loop->run();
                             });
    }

    std::unique_ptr<icp::core::event_loop> loop;
    std::vector<std::unique_ptr<api::server>> servers;
    std::thread worker;
};

}
}
extern "C" {

int icp_packetio_init(void *context, void *state)
{
    icp::packetio::service *s = reinterpret_cast<icp::packetio::service *>(state);
    s->init(context);
    s->start();
    return (0);
}

struct icp_module packetio = {
    .name = "packetio",
    .state = new icp::packetio::service(),
    .init = icp_packetio_init,
};

REGISTER_MODULE(packetio)

}
