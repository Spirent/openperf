#include <zmq.h>

#include "core/icp_core.h"
#include "packetio/api_server.h"
#include "packetio/interface_api.h"
#include "swagger/v1/model/Interface.h"

namespace icp {
namespace packetio {
namespace interface {
namespace api {

const std::string endpoint = "inproc://icp_packetio_interface";

class server : public icp::packetio::api::server::registrar<server> {
public:
    server(void *context, icp::core::event_loop &loop)
        : socket(icp_socket_get_server(context, ZMQ_REP, endpoint.c_str()))
    {
#if 0
        struct icp_event_callbacks callbacks = {
            .on_read = _handle_rpc_request
        };
        loop.add(socket.get(), &callbacks, nullptr);
#endif
    }
private:
    std::unique_ptr<void, icp_socket_deleter> socket;
};

}
}
}
}
