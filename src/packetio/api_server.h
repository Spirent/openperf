#ifndef _ICP_PACKETIO_API_SERVER_H_
#define _ICP_PACKETIO_API_SERVER_H_

#include "core/icp_event_loop.hpp"
#include "core/icp_init_factory.hpp"

namespace icp {
namespace packetio {
namespace api {

struct server : icp::core::init_factory<server, void *, icp::core::event_loop &>
{
    server(Key) {};
};

}
}
}
#endif /* _ICP_PACKETIO_API_SERVER_H_ */
