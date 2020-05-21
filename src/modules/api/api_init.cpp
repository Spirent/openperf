#include <cerrno>
#include <cstdlib>
#include <csignal>
#include <thread>

#include <netinet/in.h>

#include "pistache/endpoint.h"
#include "pistache/router.h"

#include "core/op_core.h"
#include "core/op_log.h"
#include "config/op_config_file.hpp"
#include "api/api_config_file_resources.hpp"
#include "api/api_route_handler.hpp"

namespace openperf::api {

static in_port_t service_port = 8080;

static int module_version = 1;

using namespace Pistache;

static int set_service_port(long port)
{
    if (port == 0) {
        return (EINVAL);
    } else if (port > 65535) {
        return (ERANGE);
    } else {
        service_port = static_cast<in_port_t>(port);
    }

    return (0);
}

in_port_t api_get_service_port(void) { return service_port; }

static Rest::Route::Result NotFound(const Rest::Request& request
                                    __attribute__((unused)),
                                    Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Found);
    return Rest::Route::Result::Ok;
}

int api_configure_self()
{
    auto port_num =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
            "modules.api.port");

    if (port_num) { return (set_service_port(*port_num)); }

    return (0);
}

class service
{
public:
    service() = default;
    ;

    int pre_init()
    {
        // Return 404 for anything we don't explicitly handle
        Rest::Routes::NotFound(m_router, NotFound);

        return (api_configure_self());
    }

    int post_init(void* context)
    {
        route::handler::make_all(m_handlers, context, m_router);
        return (0);
    }

    int start()
    {
        /*
         * XXX: The pistache server doesn't always shutdown, so just
         * detach from the thread so we won't be blocked on it when we
         * shutdown ourselves.
         */
        auto service_thread = std::thread([this]() { run(service_port); });
        OP_LOG(
            OP_LOG_DEBUG, "REST API server listening on port %d", service_port);
        service_thread.detach();

        /*
         * Spawn a thread to populate configuration; modules might load after us
         * and we don't want to block them.
         */
        auto config_thread = std::thread([]() {
            op_thread_setname("op_api_config");
            auto ret = config::op_config_file_process_resources();
            if (!ret) {
                throw std::runtime_error(
                    "Configuration file processing failed: " + ret.error());
            }
        });
        config_thread.detach();

        return (0);
    }

    void run(in_port_t port)
    {
        op_thread_setname("op_api");

        Address addr(IP::any(Ipv6::supported()), Port(port));
        auto opts =
            Http::Endpoint::options().threads(1).flags(Tcp::Options::ReuseAddr);

        m_server = std::make_unique<Http::Endpoint>(addr);
        m_server->init(opts);
        m_server->setHandler(m_router.handler());
        m_server->serve(); // blocks
    }

    void stop()
    {
        if (m_server) {
            m_server->shutdown();
            m_server.reset();
        }
    }

private:
    Rest::Router m_router;
    std::vector<std::unique_ptr<route::handler>> m_handlers;
    std::unique_ptr<Http::Endpoint> m_server;
};

} // namespace openperf::api

extern "C" {

int api_service_pre_init(void* context, void* state)
{
    (void)context;
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    return (s->pre_init());
}

int api_service_post_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    return (s->post_init(context));
}

int api_service_start(void* state)
{
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    return (s->start());
}

void api_service_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    s->stop();
    delete s;
}

REGISTER_MODULE(api_service,
                INIT_MODULE_INFO("api",
                                 "Module to handle API services",
                                 openperf::api::module_version),
                new openperf::api::service(),
                api_service_pre_init,
                nullptr,
                api_service_post_init,
                api_service_start,
                api_service_fini)
}
