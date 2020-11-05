#include <zmq.h>
#include <thread>

#include "core/op_core.h"
#include "packetio/init.hpp"
#include "packet/capture/server.hpp"

namespace openperf::packet::capture {

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
    std::unique_ptr<openperf::core::event_loop> m_loop;
    std::unique_ptr<api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;

    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        if (!packetio::is_enabled()) {
            OP_LOG(OP_LOG_WARNING,
                   "PacketIO module is not enabled; skipping packet capture "
                   "initialization\n");
            return;
        }

        m_loop = std::make_unique<openperf::core::event_loop>();
        m_server = std::make_unique<api::server>(context, *m_loop);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://packet_capture_shutdown_canary"));
    }

    void start()
    {
        if (!m_loop) { return; }

        m_service = std::thread([this]() {
            op_thread_setname("op_packet_cap");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);
            m_loop->run();
        });
    }
};

} // namespace openperf::packet::capture

extern "C" {

int op_capture_init(void* context, void* state)
{
    auto s = reinterpret_cast<openperf::packet::capture::service*>(state);
    s->init(context);
    return (0);
}

int op_capture_start(void* state)
{
    auto s = reinterpret_cast<openperf::packet::capture::service*>(state);
    s->start();
    return (0);
}

void op_capture_fini(void* state)
{
    auto s = reinterpret_cast<openperf::packet::capture::service*>(state);
    delete s;
}

REGISTER_MODULE(packet_capture,
                INIT_MODULE_INFO("packet-capture",
                                 "Module for capturing packets",
                                 openperf::packet::capture::module_version),
                new openperf::packet::capture::service(),
                nullptr,
                op_capture_init,
                nullptr,
                op_capture_start,
                op_capture_fini);
}
