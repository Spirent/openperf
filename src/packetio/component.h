#ifndef _ICP_PACKETIO_COMPONENT_H_
#define _ICP_PACKETIO_COMPONENT_H_

#include "core/icp_init_factory.hpp"

namespace icp {
namespace packetio {

struct component : icp::core::init_factory<component>
{
    component(Key) {};
};

}
}

#endif /* _ICP_PACKETIO_COMPONENT_H_ */
