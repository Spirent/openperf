#ifndef _ICP_PACKETIO_INTERFACE_SERVER_H_
#define _ICP_PACKETIO_INTERFACE_SERVER_H_

#include <map>
#include <memory>

#include "core/icp_core.h"

namespace swagger {
namespace v1 {
namespace model {
class Interface;
}
}
}

namespace icp {

namespace core {
class event_loop;
}

namespace packetio {
namespace interface {
namespace api {

typedef std::map<unsigned, std::shared_ptr<swagger::v1::model::Interface>> interface_map;

class server
{
public:
    server(void *context, core::event_loop& loop);

private:
    std::unique_ptr<void, icp_socket_deleter> m_socket;
    interface_map m_interfaces;
};

}
}
}
}
#endif /* _ICP_PACKETIO_INTERFACE_SERVER_H_ */
