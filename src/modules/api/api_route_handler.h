#ifndef _ICP_API_ROUTE_HANDLER_H_
#define _ICP_API_ROUTE_HANDLER_H_

#include "pistache/router.h"
#include "core/icp_init_factory.hpp"

namespace icp {
namespace api {
namespace route {

struct handler : icp::core::init_factory<handler, void *, Pistache::Rest::Router &>
{
    handler(Key) {};
    virtual ~handler() = default;
};

}
}
}

#endif /* _ICP_API_ROUTE_HANDLER_ */
