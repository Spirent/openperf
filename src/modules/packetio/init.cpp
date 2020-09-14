#include <algorithm>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <zmq.h>

#include "core/op_core.h"
#include "packetio/generic_driver.hpp"
#include "packetio/generic_stack.hpp"
#include "packetio/generic_workers.hpp"
#include "packetio/interface_server.hpp"
#include "packetio/internal_server.hpp"
#include "packetio/port_server.hpp"
#include "packetio/stack_server.hpp"

namespace openperf::packetio {

static constexpr int module_version = 1;

static int handle_zmq_shutdown(const op_event_data* data,
                               void* arg __attribute__((unused)))
{
    if (zmq_recv(data->socket, nullptr, 0, ZMQ_DONTWAIT) == -1
        && errno == ETERM) {
        op_event_loop_exit(data->loop);
    }

    return (0);
}

struct service
{
    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        m_loop = std::make_unique<openperf::core::event_loop>();
        m_driver = driver::make();
        m_workers = workers::make(context, *m_loop, *m_driver);
        m_stack = stack::make(*m_driver, *m_workers);
        m_port_server =
            std::make_unique<port::api::server>(context, *m_loop, *m_driver);
        m_stack_server =
            std::make_unique<stack::api::server>(context, *m_loop, *m_stack);
        m_if_server = std::make_unique<interface::api::server>(
            context, *m_loop, *m_stack);
        m_internal_server = std::make_unique<internal::api::server>(
            context, *m_loop, *m_workers);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://packetio_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_pio");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
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
    std::unique_ptr<openperf::core::event_loop> m_loop;
    std::unique_ptr<driver::generic_driver> m_driver;
    std::unique_ptr<workers::generic_workers> m_workers;
    std::unique_ptr<stack::generic_stack> m_stack;
    std::unique_ptr<port::api::server> m_port_server;
    std::unique_ptr<stack::api::server> m_stack_server;
    std::unique_ptr<interface::api::server> m_if_server;
    std::unique_ptr<internal::api::server> m_internal_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;
};

} // namespace openperf::packetio

extern "C" {

int op_packetio_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::packetio::service*>(state);
    s->init(context);
    s->start();
    return (0);
}

void op_packetio_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::packetio::service*>(state);
    delete s;
}

REGISTER_MODULE(packetio,
                INIT_MODULE_INFO("packetio",
                                 "Core module comprising TCP/IP stack "
                                 "functionality, virtual interfaces, and DPDK",
                                 openperf::packetio::module_version),
                new openperf::packetio::service(),
                nullptr,
                op_packetio_init,
                nullptr,
                nullptr,
                op_packetio_fini);
}
