#ifndef _ICP_PACKETIO_PGA_CLIENT_H_
#define _ICP_PACKETIO_PGA_CLIENT_H_

#include <tl/expected.hpp>

#include "packetio/pga/generic_sink.h"
#include "packetio/pga/generic_source.h"
#include "packetio/pga/pga_api.h"

#include "core/icp_core.h"

namespace icp::packetio::pga::api {

class client
{
    std::unique_ptr<void, icp_socket_deleter> m_socket;

public:
    client(void *context);

    tl::expected<void, reply_error> interface_add(std::string_view id, generic_sink& sink);
    tl::expected<void, reply_error> interface_del(std::string_view id, generic_sink& sink);

    tl::expected<void, reply_error> interface_add(std::string_view id, generic_source& source);
    tl::expected<void, reply_error> interface_del(std::string_view id, generic_source& source);

    tl::expected<void, reply_error> port_add(std::string_view id, generic_sink& sink);
    tl::expected<void, reply_error> port_del(std::string_view id, generic_sink& sink);

    tl::expected<void, reply_error> port_add(std::string_view id, generic_source& source);
    tl::expected<void, reply_error> port_del(std::string_view id, generic_source& source);
};

}
#endif /* _ICP_PACKETIO_PGA_CLIENT_H_ */
