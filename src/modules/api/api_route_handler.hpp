#ifndef _OP_API_ROUTE_HANDLER_HPP_
#define _OP_API_ROUTE_HANDLER_HPP_

#include "pistache/router.h"
#include "core/op_init_factory.hpp"

namespace openperf::api::route {

struct handler
    : openperf::core::init_factory<handler, void*, Pistache::Rest::Router&>
{
    handler(Key){};
    virtual ~handler() = default;
};

} // namespace openperf::api::route

#endif /* _OP_API_ROUTE_HANDLER_ */
