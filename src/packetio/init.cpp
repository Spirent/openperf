#include <algorithm>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <zmq.h>

#include "core/icp_core.h"
#include "packetio/generic_driver.h"
#include "packetio/interface_server.h"
#include "packetio/port_server.h"

namespace icp {
namespace packetio {

struct service {
    ~service() {
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    void init(void *context) {
        m_driver = driver::make();
        m_loop = std::make_unique<icp::core::event_loop>();
        m_port_server = std::make_unique<port::api::server>(context, *m_loop, *m_driver);
        m_if_server = std::make_unique<interface::api::server>(context, *m_loop);
    }

    void start()
    {
        m_worker = std::thread([this]() {
                                   icp_thread_setname("icp_packetio");
                                   m_loop->run();
                               });
    }

    std::unique_ptr<icp::core::event_loop> m_loop;
    std::unique_ptr<driver::generic_driver> m_driver;
    std::unique_ptr<interface::api::server> m_if_server;
    std::unique_ptr<port::api::server> m_port_server;
    std::thread m_worker;
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
