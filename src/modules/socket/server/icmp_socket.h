#ifndef _ICP_SOCKET_SERVER_ICMP_SOCKET_H_
#define _ICP_SOCKET_SERVER_ICMP_SOCKET_H_

#include <cerrno>
#include <memory>
#include <utility>

#include "tl/expected.hpp"

#include "socket/server/compat/linux/icmp.h"

#include "socket/server/allocator.h"
#include "socket/server/generic_socket.h"
#include "socket/server/raw_socket.h"

struct pbuf;

namespace icp {
namespace socket {
namespace server {

class icmp_socket : public raw_socket {
public:
    icmp_socket(icp::socket::server::allocator& allocator, int flags, int protocol);

    /* getsockopt handlers */
    tl::expected<socklen_t, int> do_getsockopt(const raw_pcb*, const api::request_getsockopt&);

    /* setsockopt handlers */
    tl::expected<void, int> do_setsockopt(raw_pcb*, const api::request_setsockopt&);

    uint32_t icmp_filter();
    
private:
    uint32_t m_icmp_filter;

};

/**
 * Generic receive function used by lwIP upon packet reception.  This
 * is added to the pcb when the raw_socket object is constructed.
 */
uint8_t icmp_receive(void*, raw_pcb*, pbuf*, const ip_addr_t*);

}
}
}

#endif /* _ICP_SOCKET_SERVER_ICMP_SOCKET_H_ */
