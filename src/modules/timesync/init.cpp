#include <memory>
#include <thread>

#include <zmq.h>

#include "core/op_core.h"
#include "timesync/server.hpp"

namespace openperf::timesync {

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
    std::unique_ptr<openperf::core::event_loop> m_loop =
        std::make_unique<openperf::core::event_loop>();
    std::unique_ptr<openperf::timesync::api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;

    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        m_server =
            std::make_unique<openperf::timesync::api::server>(context, *m_loop);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://timesync_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_timesync");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);
            m_loop->run();
        });
    }
};

} // namespace openperf::timesync

extern "C" {

int op_timesync_init(void* context, void* state)
{
    auto s = reinterpret_cast<openperf::timesync::service*>(state);
    s->init(context);
    return (0);
}

int op_timesync_start(void* state)
{
    auto s = reinterpret_cast<openperf::timesync::service*>(state);
    s->start();
    return (0);
}

void op_timesync_fini(void* state)
{
    auto s = reinterpret_cast<openperf::timesync::service*>(state);
    delete s;
}

REGISTER_MODULE(timesync,
                INIT_MODULE_INFO("timesync",
                                 "Core module for time synchronization",
                                 openperf::timesync::module_version),
                new openperf::timesync::service(),
                nullptr,
                op_timesync_init,
                nullptr,
                op_timesync_start,
                op_timesync_fini);
}
