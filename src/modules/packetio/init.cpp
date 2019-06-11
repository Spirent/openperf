#include <algorithm>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <zmq.h>

#include "core/icp_core.h"
#include "packetio/generic_driver.h"
#include "packetio/generic_stack.h"
#include "packetio/interface_server.h"
#include "packetio/port_server.h"
#include "packetio/stack_server.h"
#include "packetio/pga/pga_server.h"

namespace icp {
namespace packetio {

static int module_version = 1;

struct service {
    ~service() {
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    void init(void *context) {
        m_driver = driver::make(context);
        m_stack = stack::make(*m_driver);
        m_loop = std::make_unique<icp::core::event_loop>();
        m_port_server = std::make_unique<port::api::server>(context, *m_loop, *m_driver);
        m_stack_server = std::make_unique<stack::api::server>(context, *m_loop, *m_stack);
        m_if_server = std::make_unique<interface::api::server>(context, *m_loop, *m_stack);
        m_pga_server = std::make_unique<pga::api::server>(context, *m_loop, *m_driver, *m_stack);
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
    std::unique_ptr<stack::generic_stack> m_stack;
    std::unique_ptr<port::api::server> m_port_server;
    std::unique_ptr<stack::api::server> m_stack_server;
    std::unique_ptr<interface::api::server> m_if_server;
    std::unique_ptr<pga::api::server> m_pga_server;
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

void icp_packetio_fini(void *state)
{
    icp::packetio::service *s = reinterpret_cast<icp::packetio::service*>(state);
    delete s;
}

REGISTER_MODULE(packetio,
                INIT_MODULE_INFO(
                                 "packetio",
                                 "Core module comprising TCP/IP stack functionality, virtual interfaces, and DPDK",
                                 icp::packetio::module_version
                                 ),
                new icp::packetio::service(),
                nullptr,
                icp_packetio_init,
                nullptr,
                nullptr,
                icp_packetio_fini);
}
