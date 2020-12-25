#include <memory>
#include <thread>

#include <zmq.h>

#include "config/op_config_file.hpp"
#include "framework/core/op_core.h"
#include "framework/core/op_thread.h"
#include "server.hpp"

namespace openperf::network {

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
            std::make_unique<openperf::network::api::server>(context, *m_loop);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://network_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_network");

            auto mask =
                openperf::config::file::op_config_get_param<OP_OPTION_TYPE_HEX>(
                    "modules.network.cpu-mask");
            if (mask) {
                if (auto res =
                        op_thread_set_relative_affinity_mask(mask.value());
                    res) {
                    op_exit(
                        "Applying Network module affinity mask failed! %s\n",
                        std::strerror(res));
                }

                OP_LOG(OP_LOG_DEBUG,
                       "Network module been configured with cpu-mask: 0x%x",
                       mask.value());
            }

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);

            m_loop->run();
        });
    }

    std::unique_ptr<openperf::core::event_loop> m_loop =
        std::make_unique<openperf::core::event_loop>();
    std::unique_ptr<openperf::network::api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;
};

} // namespace openperf::network

extern "C" {

int op_network_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::network::service*>(state);
    s->init(context);
    s->start();
    return 0;
}

void op_network_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::network::service*>(state);
    delete s;
}

REGISTER_MODULE(
    network,
    INIT_MODULE_INFO("network",
                     "Core module comprising network generator stack",
                     openperf::network::module_version),
    new openperf::network::service(),
    nullptr,
    op_network_init,
    nullptr,
    nullptr,
    op_network_fini);
}
