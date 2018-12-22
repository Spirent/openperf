#include "json.hpp"

#include "api/api_route_handler.h"

namespace icp {
namespace api {

using namespace Pistache;
using json = nlohmann::json;

#ifndef VERSION_COMMIT
#error VERSION_COMMIT must be defined
#endif

#ifndef VERSION_NUMBER
#error VERSION_NUMBER must be defined
#endif

#ifndef VERSION_TIMESTAMP
#error VERSION_TIMESTAMP must be defined
#endif

static
Rest::Route::Result version(const Rest::Request& request __attribute__((unused)),
                            Http::ResponseWriter response)
{
    json version = {
        { "build_number",  VERSION_NUMBER    },
        { "build_time",    VERSION_TIMESTAMP },
        { "source_commit", VERSION_COMMIT    }
    };

    response.send(Http::Code::Ok, version.dump());

    return (Rest::Route::Result::Ok);
}

class handler : public icp::api::route::handler::registrar<handler> {
public:
    handler(void *context, Rest::Router& router);
};

handler::handler(void *context __attribute__((unused)),
                 Rest::Router& router)
{
    Rest::Routes::Get(router, "/version", version);
}

}
}
