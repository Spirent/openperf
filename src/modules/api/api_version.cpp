#include "json.hpp"

#include "api/api_route_handler.hpp"

namespace openperf::api {

using namespace Pistache;
using json = nlohmann::json;

#ifndef BUILD_COMMIT
#error BUILD_COMMIT must be defined
#endif

#ifndef BUILD_NUMBER
#error BUILD_NUMBER must be defined
#endif

#ifndef BUILD_TIMESTAMP
#error BUILD_TIMESTAMP must be defined
#endif

#ifndef BUILD_VERSION
#error BUILD_VERSION must be defined
#endif

static Rest::Route::Result version(const Rest::Request& request
                                   __attribute__((unused)),
                                   Http::ResponseWriter response)
{
    json version = {{"build_number", BUILD_NUMBER},
                    {"build_time", BUILD_TIMESTAMP},
                    {"source_commit", BUILD_COMMIT},
                    {"version", BUILD_VERSION}};

    response.send(Http::Code::Ok, version.dump());

    return (Rest::Route::Result::Ok);
}

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Rest::Router& router);
};

handler::handler(void* context __attribute__((unused)), Rest::Router& router)
{
    Rest::Routes::Get(router, "/version", version);
}

} // namespace openperf::api
