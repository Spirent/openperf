#include <cerrno>
#include <cstdlib>
#include <thread>

#include <netinet/in.h>

#include "pistache/endpoint.h"
#include "pistache/router.h"

#include "core/icp_core.h"
#include "api/api_route.h"

namespace icp {
namespace api {

static in_port_t service_port = 8080;

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

using namespace Pistache;

static
Rest::Route::Result NotFound(const Rest::Request &request, Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Found);
    return Rest::Route::Result::Ok;
}

int service_init(void *context)
{
    (void)context;

    Address addr(Ipv4::any(), Port(service_port));
    auto opts = Http::Endpoint::options().threads(1);
    Rest::Router *router = new Rest::Router();

    route::registry::init_router(*router);

    Rest::Routes::NotFound(*router, NotFound);

    Http::Endpoint *server = new Http::Endpoint(addr);
    server->init(opts);
    server->setHandler(router->handler());
    server->serveThreaded();

    return (0);
}

}
}

extern "C" {

int api_option_handler(int opt, const char *opt_arg, void *opt_data __attribute__((unused)))
{
    return(icp::api::handle_options(opt, opt_arg));
}

int api_service_init(void *context)
{
    return (icp::api::service_init(context));
}
}
