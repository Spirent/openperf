#include <algorithm>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <zmq.h>

#include "core/op_core.h"
#include "packetio/generic_driver.hpp"
#include "packetio/generic_workers.hpp"
#include "packetio/internal_server.hpp"
#include "packetio/port_server.hpp"

namespace openperf::packetio {

static constexpr int module_version = 1;

static std::atomic_bool init_flag = false;

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

    bool init(void* context)
    {
        try {
            m_driver = driver::make();
        } catch (const std::exception& e) {
            OP_LOG(OP_LOG_CRITICAL,
                   "Failed to initialize PacketIO driver.  %s",
                   e.what());
            // Exit process if driver throws an exception.
            // This was added to allow restarting the process and trying again.
            // Note: sleep() allows final log message to get logged
            sleep(1);
            exit(EXIT_FAILURE);
        }

        if (!m_driver->is_usable()) {
            OP_LOG(OP_LOG_WARNING,
                   "No usable PacketIO driver available; skipping "
                   "initialization\n");
            m_driver.reset();
            return (false);
        }

        m_loop = std::make_unique<openperf::core::event_loop>();
        m_workers = workers::make(context, *m_loop, *m_driver);
        m_port_server =
            std::make_unique<port::api::server>(context, *m_loop, *m_driver);
        m_internal_server = std::make_unique<internal::api::server>(
            context, *m_loop, *m_driver, *m_workers);

        m_shutdown.reset(op_socket_get_server(
            context, ZMQ_REQ, "inproc://packetio_shutdown_canary"));

        init_flag.store(true, std::memory_order_release);

        return (true);
    }

    void start()
    {
        assert(m_driver && m_driver->is_usable());

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
    std::unique_ptr<port::api::server> m_port_server;
    std::unique_ptr<internal::api::server> m_internal_server;
    std::unique_ptr<void, op_socket_deleter> m_shutdown;
    std::thread m_service;
};

bool is_enabled() { return (init_flag.load(std::memory_order_relaxed)); }

} // namespace openperf::packetio

extern "C" {

int op_packetio_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::packetio::service*>(state);
    if (s->init(context)) { s->start(); }
    return (0);
}

void op_packetio_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::packetio::service*>(state);
    delete s;
}

REGISTER_MODULE(
    packetio,
    INIT_MODULE_INFO("packetio",
                     "Core module comprising packet source/sink handling "
                     "for network interfaces",
                     openperf::packetio::module_version),
    new openperf::packetio::service(),
    op_packetio_init,
    nullptr,
    nullptr,
    nullptr,
    op_packetio_fini);
}
