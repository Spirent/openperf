#include <algorithm>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <zmq.h>

#include "core/icp_core.h"
#include "packetio/generic_driver.h"
#include "packetio/generic_stack.h"
#include "packetio/generic_workers.h"
#include "packetio/interface_server.h"
#include "packetio/internal_server.h"
#include "packetio/port_server.h"
#include "packetio/stack_server.h"

namespace icp {
namespace packetio {

static constexpr int module_version = 1;

static int handle_zmq_shutdown(const icp_event_data *data,
                               void *arg __attribute__((unused)))
{
    if (zmq_recv(data->socket, nullptr, 0, ZMQ_DONTWAIT) == -1
        && errno == ETERM) {
        icp_event_loop_exit(data->loop);
    }

    return (0);
}

struct service {
    ~service() {
        if (m_service.joinable()) {
            m_service.join();
        }
    }

    void init(void *context) {
        m_driver = driver::make();
        m_workers = workers::make(context, *m_driver);
        m_stack = stack::make(*m_driver, *m_workers);
        m_loop = std::make_unique<icp::core::event_loop>();
        m_port_server = std::make_unique<port::api::server>(context, *m_loop, *m_driver);
        m_stack_server = std::make_unique<stack::api::server>(context, *m_loop, *m_stack);
        m_if_server = std::make_unique<interface::api::server>(context, *m_loop, *m_stack);
        m_internal_server = std::make_unique<internal::api::server>(context, *m_loop, *m_workers);

        m_shutdown.reset(icp_socket_get_server(context, ZMQ_REQ,
                                               "inproc://packetio_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
                                   icp_thread_setname("icp_packetio");

                                   struct icp_event_callbacks callbacks = {
                                       .on_read = handle_zmq_shutdown
                                   };
                                   m_loop->add(m_shutdown.get(), &callbacks, nullptr);

                                   m_loop->run();
                               });
    }

    /*
     * Note: the order of declaration here is the same as the order of
     * initialization in init().  This is both intentional and necessary
     * as the objects will be destroyed in the reverse order of their
     * declaration.
     */
    std::unique_ptr<driver::generic_driver> m_driver;
    std::unique_ptr<workers::generic_workers> m_workers;
    std::unique_ptr<stack::generic_stack> m_stack;
    std::unique_ptr<icp::core::event_loop> m_loop;
    std::unique_ptr<port::api::server> m_port_server;
    std::unique_ptr<stack::api::server> m_stack_server;
    std::unique_ptr<interface::api::server> m_if_server;
    std::unique_ptr<internal::api::server> m_internal_server;
    std::unique_ptr<void, icp_socket_deleter> m_shutdown;
    std::thread m_service;
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
