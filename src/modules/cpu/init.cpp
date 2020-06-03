#include <memory>
#include <thread>

#include <zmq.h>

#include "core/op_core.h"
#include "cpu/server.hpp"

namespace openperf::cpu {

static constexpr int module_version = 1;

static int handle_zmq_shutdown(const op_event_data* data,
                               void* arg __attribute__((unused)))
{
    if (zmq_recv(data->socket, nullptr, 0, ZMQ_DONTWAIT) == -1
        && errno == ETERM) {
        op_event_loop_exit(data->loop);
    }

    return 0;
}

struct service
{
    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        m_server =
            std::make_unique<openperf::cpu::api::server>(context, *m_loop);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://cpu_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_cpu");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);

            m_loop->run();
        });
    }

    std::unique_ptr<openperf::core::event_loop> m_loop =
        std::make_unique<openperf::core::event_loop>();
    std::unique_ptr<openperf::cpu::api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;
};

} // namespace openperf::cpu

extern "C" {

int op_cpu_init(void* context, void* state)
{
    openperf::cpu::service* s =
        reinterpret_cast<openperf::cpu::service*>(state);
    s->init(context);
    s->start();
    return 0;
}

void op_cpu_fini(void* state)
{
    openperf::cpu::service* s =
        reinterpret_cast<openperf::cpu::service*>(state);
    delete s;
}

REGISTER_MODULE(cpu,
                INIT_MODULE_INFO("cpu",
                                 "Core module comprising cpu generator stack",
                                 openperf::cpu::module_version),
                new openperf::cpu::service(),
                nullptr,
                op_cpu_init,
                nullptr,
                nullptr,
                op_cpu_fini);
}
