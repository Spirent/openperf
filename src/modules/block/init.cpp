#include <memory>
#include <thread>

#include <zmq.h>

#include "server.hpp"

#include "config/op_config_file.hpp"
#include "framework/core/op_core.h"

extern const char op_block_mask[];

namespace openperf::block {

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

static std::optional<core::cpuset> core_mask()
{
    if (const auto mask = openperf::config::file::op_config_get_param<
            OP_OPTION_TYPE_CPUSET_STRING>(op_block_mask)) {
        return (core::cpuset_online() & core::cpuset(mask.value()));
    }

    return (std::nullopt);
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
            std::make_unique<openperf::block::api::server>(context, *m_loop);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://block_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_block");

            if (auto mask = core_mask()) {
                if (auto error = core::cpuset_set_affinity(mask.value())) {
                    OP_LOG(OP_LOG_ERROR,
                           "Could not configure %s as core mask for block "
                           "generator: %s\n",
                           mask->to_string().c_str(),
                           strerror(error));
                } else {
                    OP_LOG(OP_LOG_DEBUG,
                           "Configured %s as core mask for block generator\n",
                           mask->to_string().c_str());
                }
            }

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};
            m_loop->add(m_shutdown.get(), &callbacks, nullptr);

            m_loop->run();
        });
    }

    std::unique_ptr<openperf::core::event_loop> m_loop =
        std::make_unique<openperf::core::event_loop>();
    std::unique_ptr<openperf::block::api::server> m_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;
};

} // namespace openperf::block

extern "C" {

int op_block_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::block::service*>(state);
    s->init(context);
    s->start();
    return (0);
}

void op_block_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::block::service*>(state);
    delete s;
}

REGISTER_MODULE(block,
                INIT_MODULE_INFO("block",
                                 "Core module comprising block generator stack",
                                 openperf::block::module_version),
                new openperf::block::service(),
                nullptr,
                op_block_init,
                nullptr,
                nullptr,
                op_block_fini);
}
