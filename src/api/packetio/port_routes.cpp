#include "pistache/router.h"

#include "core/icp_core.h"
#include "api/api_route.h"

namespace icp {
namespace api {
namespace packetio {

using namespace Pistache;

Rest::Route::Result ListPorts(const Rest::Request &request, Http::ResponseWriter response)
{
    icp_log(ICP_LOG_TRACE, "Received request\n");
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

Rest::Route::Result CreatePort(const Rest::Request &request, Http::ResponseWriter response)
{
    icp_log(ICP_LOG_TRACE, "Received request\n");
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

Rest::Route::Result GetPort(const Rest::Request &request, Http::ResponseWriter response)
{
    auto port_idx = request.param(":id").as<int>();
    icp_log(ICP_LOG_TRACE, "Received GET port request for idx = %d\n", port_idx);
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

Rest::Route::Result PutPort(const Rest::Request &request, Http::ResponseWriter response)
{
    auto port_idx = request.param(":id").as<int>();
    icp_log(ICP_LOG_TRACE, "Received PUT port request for idx = %d\n", port_idx);
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

std::initializer_list<icp::api::route::descriptor> port_routes = {
    { icp::api::route::operation::http_get,  "ports",     ListPorts },
    { icp::api::route::operation::http_post, "ports",     CreatePort },
    { icp::api::route::operation::http_get,  "ports/:id", GetPort },
    { icp::api::route::operation::http_put,  "ports/:id", PutPort }
};

REGISTER_API_ROUTES(port_routes);

}
}
}
