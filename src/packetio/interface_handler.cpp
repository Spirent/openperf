#include <zmq.h>

#include "api/api_route_handler.h"
#include "core/icp_core.h"
#include "packetio/interface_api.h"

namespace icp {
namespace packetio {
namespace interface {
namespace api {

using namespace Pistache;
using json = nlohmann::json;

class handler : public icp::api::route::handler::registrar<handler> {
public:
    handler(void *context, Rest::Router &router);

    Rest::Route::Result  list_interfaces(const Rest::Request &request, Http::ResponseWriter response);
    Rest::Route::Result create_interface(const Rest::Request &request, Http::ResponseWriter response);
    Rest::Route::Result    get_interface(const Rest::Request &request, Http::ResponseWriter response);
    Rest::Route::Result delete_interface(const Rest::Request &request, Http::ResponseWriter response);

private:
    std::unique_ptr<void, icp_socket_deleter> socket;
};

handler::handler(void *context, Rest::Router &router)
    : socket(icp_socket_get_client(context, ZMQ_REQ, endpoint.c_str()))
{
    Rest::Routes::Get(router, "/interfaces",
                      Rest::Routes::bind(&handler::list_interfaces, this));
    Rest::Routes::Post(router, "/interfaces",
                       Rest::Routes::bind(&handler::create_interface, this));
    Rest::Routes::Get(router, "/interfaces/:id",
                      Rest::Routes::bind(&handler::get_interface, this));
    Rest::Routes::Delete(router, "/interfaces/:id",
                         Rest::Routes::bind(&handler::delete_interface, this));
}

Rest::Route::Result handler::list_interfaces(const Rest::Request &request,
                                             Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

Rest::Route::Result handler::create_interface(const Rest::Request &request,
                                              Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

Rest::Route::Result handler::get_interface(const Rest::Request &request,
                                           Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

Rest::Route::Result handler::delete_interface(const Rest::Request &request,
                                              Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Implemented);
    return (Rest::Route::Result::Ok);
}

}
}
}
}
