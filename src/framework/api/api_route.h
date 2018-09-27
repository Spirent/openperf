#ifndef _ICP_API_ROUTE_H_
#define _ICP_API_ROUTE_H_

#include <initializer_list>
#include <string>

#include "pistache/router.h"

namespace icp {
namespace api {
namespace route {

enum class operation {none, http_get, http_put, http_post, http_delete};

/**
 * Describes a REST route
 */
struct descriptor {
    operation op;
    std::string route;
    Pistache::Rest::Route::Handler handler;
};

/*
 * Provide a mechanism for code to register REST routes for our endpoint.
 * Yep, it's a singleton; get over it.
 */
class registry
{
public:
    static registry& instance();
    static void init_router(Pistache::Rest::Router &router);
    void add_route(operation op, std::string route, Pistache::Rest::Route::Handler handler);
private:
    registry() {}
    std::vector<descriptor> _descriptors;
};

struct registry_helper
{
    registry_helper(std::initializer_list<descriptor> &descriptors);
};

}
}
}

#define REGISTER_API_ROUTES(d)                              \
    icp::api::route::registry_helper d##_registry_helper(d) \

#endif /* _ICP_API_ROUTE_H_ */
