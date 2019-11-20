#ifndef _OP_SOCKET_SERVER_ICMP_SOCKET_H_
#define _OP_SOCKET_SERVER_ICMP_SOCKET_H_

#include <bitset>

#include "tl/expected.hpp"

#include "socket/server/compat/linux/icmp.h"

#include "socket/server/allocator.h"
#include "socket/server/generic_socket.h"
#include "socket/server/raw_socket.h"

struct pbuf;

namespace openperf {
namespace socket {
namespace server {

class icmp_socket : public raw_socket {
public:
    icmp_socket(openperf::socket::server::allocator& allocator, int flags, int protocol);
    ~icmp_socket() = default;

    icmp_socket(const icmp_socket&) = delete;
    icmp_socket& operator=(const icmp_socket&&) = delete;

    icmp_socket& operator=(icmp_socket&& other) noexcept;
    icmp_socket(icmp_socket&& other) noexcept;

    /* getsockopt handlers */
    tl::expected<socklen_t, int> do_getsockopt(const raw_pcb*, const api::request_getsockopt&);

    /* setsockopt handlers */
    tl::expected<void, int> do_setsockopt(raw_pcb*, const api::request_setsockopt&);

    bool is_filtered(uint8_t icmp_type) const;

private:
    /*
     * The filter can filter any ICMP type, which is defined by a single octet.
     * If bit n in the bitset is set, then type n should be filtered.
     */
    using icmp_filter = std::bitset<256>;
    icmp_filter m_filter;
};

}
}
}

#endif /* _OP_SOCKET_SERVER_ICMP_SOCKET_H_ */
