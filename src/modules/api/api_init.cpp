#include <cerrno>
#include <cstdlib>
#include <thread>

#include <netinet/in.h>

#include "pistache/endpoint.h"
#include "pistache/router.h"

#include "core/icp_core.h"
#include "api/api_route_handler.h"

namespace icp {
namespace api {

static in_port_t service_port = 8080;

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
    ~service()
    {
        if (_service.joinable()) {
            _service.join();
        }
    };

    int pre_init() {
        // Return 404 for anything we don't explicitly handle
        Rest::Routes::NotFound(_router, NotFound);

        return (0);
    }

    int post_init(void *context) {
        route::handler::make_all(_handlers, context, _router);
        return (0);
    }

    int start() {
        _service = std::thread([this]() { run(service_port); });
        return (0);
    }

    void run(in_port_t port) {
        icp_thread_setname("icp_api");

        Address addr(Ipv4::any(), Port(port));
        auto opts = Http::Endpoint::options()
            .threads(1)
            .flags(Tcp::Options::ReuseAddr);

        Http::Endpoint *server = new Http::Endpoint(addr);
        server->init(opts);
        server->setHandler(_router.handler());
        server->serve();  // blocks
        server->shutdown();
    }

private:
    Rest::Router _router;
    std::thread _service;
    std::vector<std::unique_ptr<route::handler>> _handlers;
};

}
}

extern "C" {

int api_option_handler(int opt, const char *opt_arg, void *opt_data __attribute__((unused)))
{
    return (icp::api::handle_options(opt, opt_arg));
}

int api_service_pre_init(void *context, void *state)
{
    (void)context;
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->pre_init());
}

int api_server_post_init(void *context, void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->post_init(context));
}

int api_service_start(void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->start());
}

struct icp_module api_service = {
    .name = "API service",
    .state = new icp::api::service(),
    .pre_init = api_service_pre_init,
    .post_init = api_server_post_init,
    .start = api_service_start,
};

REGISTER_MODULE(api_service)

}
