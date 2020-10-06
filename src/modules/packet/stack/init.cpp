#include <thread>

#include <zmq.h>

#include "core/op_core.h"
#include "packet/stack/generic_stack.hpp"
#include "packet/stack/server.hpp"

namespace openperf::packet::stack {

static constexpr int module_version = 1;

static int handle_zmq_shutdown(const op_event_data* data, void*)
{
    if (zmq_recv(data->socket, nullptr, 0, ZMQ_DONTWAIT) == -1
        && errno == ETERM) {
        op_event_loop_exit(data->loop);
    }

    return (0);
}

struct service
{
    std::unique_ptr<openperf::core::event_loop> m_loop =
        std::make_unique<openperf::core::event_loop>();
    std::unique_ptr<generic_stack> m_stack;
    std::unique_ptr<api::server> m_stack_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;

    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        m_stack = stack::make(context);
        m_stack_server =
            std::make_unique<api::server>(context, *m_loop, *m_stack);
        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://packet_stack_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_packet_stack");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);

            m_loop->run();
        });
    }
};

} // namespace openperf::packet::stack

extern "C" {

int op_stack_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::packet::stack::service*>(state);
    s->init(context);
    return (0);
}

int op_stack_start(void* state)
{
    auto* s = reinterpret_cast<openperf::packet::stack::service*>(state);
    s->start();
    return (0);
}

void op_stack_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::packet::stack::service*>(state);
    delete s;
}

REGISTER_MODULE(
    packet_stack,
    INIT_MODULE_INFO("packet-stack",
                     "This module provides TCP/IP stack functionality",
                     openperf::packet::stack::module_version),
    new openperf::packet::stack::service(),
    nullptr,
    op_stack_init,
    nullptr,
    op_stack_start,
    op_stack_fini);
}
