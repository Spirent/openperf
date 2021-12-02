#include <thread>
#include <zmq.h>

#include "server.hpp"
#include "framework/core/op_core.h"
#include "config/op_config_file.hpp"

extern const char op_memory_mask[];

namespace openperf::memory {

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

static std::optional<core::cpuset> core_mask()
{
    if (const auto mask = openperf::config::file::op_config_get_param<
            OP_OPTION_TYPE_CPUSET_STRING>(op_memory_mask)) {
        return (core::cpuset_online() & core::cpuset(mask.value()));
    }

    return (std::nullopt);
}

class service
{
private:
    std::thread m_service;

    std::unique_ptr<openperf::core::event_loop> m_loop;
    std::unique_ptr<openperf::memory::api::server> m_server;
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
            std::make_unique<openperf::memory::api::server>(context, *m_loop);
        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://memory_shutdown_canary"));
    }

    void start()
    {
        m_service = std::thread([this]() {
            op_thread_setname("op_memory");

            if (auto mask = core_mask()) {
                if (auto error = core::cpuset_set_affinity(mask.value())) {
                    OP_LOG(OP_LOG_ERROR,
                           "Could not configure %s as core mask for memory "
                           "generator: %s\n",
                           mask->to_string().c_str(),
                           strerror(error));
                } else {
                    OP_LOG(OP_LOG_DEBUG,
                           "Configured %s as core mask for memory generator\n",
                           mask->to_string().c_str());
                }
            }

            struct op_event_callbacks callbacks = {.on_read =
                                                       handle_zmq_shutdown};

            m_loop->add(m_shutdown.get(), &callbacks, nullptr);
            m_loop->run();
        });
    }
};

} // namespace openperf::memory

extern "C" {

int memory_generator_init(void* context, void* state)
{
    auto s = reinterpret_cast<openperf::memory::service*>(state);
    s->init(context);
    return 0;
}

int memory_generator_start(void* state)
{
    auto s = reinterpret_cast<openperf::memory::service*>(state);
    s->start();
    return 0;
}

void memory_generator_fini(void* state)
{
    auto s = reinterpret_cast<openperf::memory::service*>(state);
    delete s;
}

REGISTER_MODULE(memgen,
                INIT_MODULE_INFO("memory",
                                 "Module for make memory load",
                                 openperf::memory::module_version),
                new openperf::memory::service(),
                nullptr,
                memory_generator_init,
                nullptr,
                memory_generator_start,
                memory_generator_fini);

} // extern "C"
