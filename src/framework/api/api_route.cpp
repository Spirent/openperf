#include "pistache/router.h"

#include "core/icp_core.h"
#include "api/api_route.h"

namespace icp {
namespace api {
namespace route {

using namespace Pistache::Rest;

//static
registry& registry::instance()
{
    static registry instance;
    return instance;
}

//static
void registry::init_router(Router &router)
{
    auto &inst = instance();
    for (auto &d : inst._descriptors) {
        switch (d.op) {
        case operation::http_get:
            Routes::Get(router, d.route, d.handler);
            break;
        case operation::http_put:
            Routes::Put(router, d.route, d.handler);
            break;
        case operation::http_post:
            Routes::Post(router, d.route, d.handler);
            break;
        case operation::http_delete:
            Routes::Delete(router, d.route, d.handler);
            break;
        default:
            icp_exit("Unsupported route operation: %d\n", d.op);
        }
    }
}

void registry::add_route(operation op, std::string route, Route::Handler handler)
{
    _descriptors.push_back({op, route, handler});
}

registry_helper::registry_helper(std::initializer_list<descriptor> &descriptors)
{
    for (auto &d : descriptors) {
        registry::instance().add_route(d.op, d.route, d.handler);
    }
}

}
}
}
