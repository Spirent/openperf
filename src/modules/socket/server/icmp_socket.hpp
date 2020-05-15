#ifndef _OP_SOCKET_SERVER_ICMP_SOCKET_HPP_
#define _OP_SOCKET_SERVER_ICMP_SOCKET_HPP_

#include <bitset>

#include "tl/expected.hpp"

#include "socket/server/allocator.hpp"
#include "socket/server/generic_socket.hpp"
#include "socket/server/raw_socket.hpp"

struct pbuf;

namespace openperf::socket::server {

class icmp_socket : public raw_socket
{
public:
    icmp_socket(openperf::socket::server::allocator& allocator,
                enum lwip_ip_addr_type ip_type,
                int flags,
                int protocol);
    ~icmp_socket() = default;

    icmp_socket(const icmp_socket&) = delete;
    icmp_socket& operator=(const icmp_socket&&) = delete;

    icmp_socket& operator=(icmp_socket&& other) noexcept;
    icmp_socket(icmp_socket&& other) noexcept;

    /* getsockopt handlers */
    tl::expected<socklen_t, int>
    do_getsockopt(const raw_pcb*, const api::request_getsockopt&) override;

    /* setsockopt handlers */
    tl::expected<void, int>
    do_setsockopt(raw_pcb*, const api::request_setsockopt&) override;

    bool is_filtered(uint8_t icmp_type) const;

private:
    /*
     * The filter can filter any ICMP type, which is defined by a single octet.
     * If bit n in the bitset is set, then type n should be filtered.
     */
    using icmp_filter = std::bitset<256>;
    icmp_filter m_filter;

    tl::expected<socklen_t, int>
    do_icmp6_getsockopt(const raw_pcb*, const api::request_getsockopt&);

    tl::expected<void, int> do_icmp6_setsockopt(raw_pcb*,
                                                const api::request_setsockopt&);
};

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_ICMP_SOCKET_HPP_ */
