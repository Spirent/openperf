#include <cerrno>
#include <cstdlib>
#include <csignal>
#include <thread>

#include <netinet/in.h>

#include "pistache/endpoint.h"
#include "pistache/router.h"

#include "core/icp_core.h"
#include "api/api_route_handler.h"

namespace icp {
namespace api {

static in_port_t service_port = 8080;

static int module_version = 1;

using namespace Pistache;

static int handle_options(int opt, const char *opt_arg)
{
    if (opt != 'p' || opt_arg == nullptr) {
        return (-EINVAL);
    }

    long p = std::strtol(opt_arg, nullptr, 10);

    /* Check return value */
    if (p == 0) {
        return (-EINVAL);
    } else if (p > 65535) {
        return (-ERANGE);
    } else {
        service_port = static_cast<in_port_t>(p);
    }

    return (0);
}

in_port_t api_get_service_port(void)
{
    return service_port;
}

static
Rest::Route::Result NotFound(const Rest::Request &request __attribute__((unused)),
                             Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Found);
    return Rest::Route::Result::Ok;
}

class service {
public:
    service() {};

    int pre_init() {
        // Return 404 for anything we don't explicitly handle
        Rest::Routes::NotFound(m_router, NotFound);

        return (0);
    }

    int post_init(void *context) {
        route::handler::make_all(m_handlers, context, m_router);
        return (0);
    }

    int start() {
        /*
         * XXX: The pistache server doesn't always shutdown, so just
         * detach from the thread so we won't be blocked on it when we
         * shutdown ourselves.
         */
        auto thread = std::thread([this]() { run(service_port); });
        thread.detach();
        return (0);
    }

    void run(in_port_t port) {
        icp_thread_setname("icp_api");

        Address addr(Ipv4::any(), Port(port));
        auto opts = Http::Endpoint::options()
            .threads(1)
            .flags(Tcp::Options::ReuseAddr);

        m_server = std::make_unique<Http::Endpoint>(addr);
        m_server->init(opts);
        m_server->setHandler(m_router.handler());
        m_server->serve();        // blocks
    }

    void stop() {
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

}
}

extern "C" {

int api_option_handler(int opt, const char *opt_arg)
{
    return (icp::api::handle_options(opt, opt_arg));
}

int api_service_pre_init(void *context, void *state)
{
    (void)context;
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->pre_init());
}

int api_service_post_init(void *context, void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->post_init(context));
}

int api_service_start(void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->start());
}

void api_service_fini(void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    s->stop();
    delete s;
}

REGISTER_MODULE(api_service,
                INIT_MODULE_INFO(
                                 "api",
                                 "Module to handle API services",
                                 icp::api::module_version
                                 ),
                new icp::api::service(),
                api_service_pre_init,
                nullptr,
                api_service_post_init,
                api_service_start,
                api_service_fini)
}
