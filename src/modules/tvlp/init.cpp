#include <thread>
#include <zmq.h>

#include "server.hpp"
#include "framework/core/op_core.h"

namespace openperf::tvlp {

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

class service
{
private:
    std::thread m_service;

    std::unique_ptr<openperf::core::event_loop> m_loop;
    std::unique_ptr<openperf::tvlp::api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;

public:
    ~service()
    {
        if (m_service.joinable()) { m_service.join(); }
    }

    void init(void* context)
    {
        m_loop = std::make_unique<openperf::core::event_loop>();
        m_server =
            std::make_unique<openperf::tvlp::api::server>(context, *m_loop);
        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://tvlp_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_tvlp");

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};

            m_loop->add(m_shutdown.get(), &callbacks, nullptr);
            m_loop->run();
        });
    }
};

} // namespace openperf::tvlp

extern "C" {

int tvlp_init(void* context, void* state)
{
    auto s = reinterpret_cast<openperf::tvlp::service*>(state);
    s->init(context);
    return 0;
}

int tvlp_start(void* state)
{
    auto s = reinterpret_cast<openperf::tvlp::service*>(state);
    s->start();
    return 0;
}

void tvlp_fini(void* state)
{
    auto s = reinterpret_cast<openperf::tvlp::service*>(state);
    delete s;
}

static constexpr op_module_info tvlp_module_info =
    INIT_MODULE_INFO("tvlp", "TVLP module", openperf::tvlp::module_version);

REGISTER_PLUGIN_MODULE(tvlp_m,
                       tvlp_module_info,
                       new openperf::tvlp::service(),
                       nullptr,
                       tvlp_init,
                       nullptr,
                       tvlp_start,
                       tvlp_fini);

} // extern "C"
