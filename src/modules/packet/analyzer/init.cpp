#include <zmq.h>

#include "core/op_core.h"
#include "packet/analyzer/server.hpp"

namespace openperf::packet::analyzer {

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
    std::unique_ptr<api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;

    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        m_server = std::make_unique<api::server>(context, *m_loop);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://packet_analyzer_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_packet_ana");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);
            m_loop->run();
        });
    }
};

} // namespace openperf::packet::analyzer

extern "C" {

int op_analyzer_init(void* context, void* state)
{
    auto s = reinterpret_cast<openperf::packet::analyzer::service*>(state);
    s->init(context);
    return (0);
}

int op_analyzer_start(void* state)
{
    auto s = reinterpret_cast<openperf::packet::analyzer::service*>(state);
    s->start();
    return (0);
}

void op_analyzer_fini(void* state)
{
    auto s = reinterpret_cast<openperf::packet::analyzer::service*>(state);
    delete s;
}

REGISTER_MODULE(
    packet_analyzer,
    INIT_MODULE_INFO("packet-analyzer",
                     "Module that reports network packet statistics",
                     openperf::packet::analyzer::module_version),
    new openperf::packet::analyzer::service(),
    nullptr,
    op_analyzer_init,
    nullptr,
    op_analyzer_start,
    op_analyzer_fini);
}
