#ifndef _OP_SOCKET_DGRAM_CHANNEL_HPP_
#define _OP_SOCKET_DGRAM_CHANNEL_HPP_

#include <atomic>

#include <netinet/in.h>

#include "framework/memory/offset_ptr.hpp"
#include "socket/api.hpp"

namespace openperf::socket {

struct dgram_ipv4_addr
{
    uint32_t addr;
};

struct dgram_ipv6_addr
{
    uint32_t addr[4];
    /* LWIP_IPV6_SCOPES adds zone field in lwpip ip6_addr_t */
    uint8_t zone;
};

/* Congruent to lwIP ip_addr_t; otherwise we'd use a variant */
struct dgram_ip_addr
{
    enum class type : uint8_t { IPv4 = 0, IPv6 = 6 };
    union
    {
        dgram_ipv6_addr ip6;
        dgram_ipv4_addr ip4;
    } u_addr;
    type type;
};

/* Congruent to struct sockaddr_ll */
struct dgram_sockaddr_ll
{
    uint16_t family;
    uint16_t protocol;
    int ifindex;
    uint16_t hatype;
    uint8_t pkttype;
    uint8_t halen;
    std::array<uint8_t, 8> addr;
};

using dgram_channel_addr =
    std::variant<sockaddr_in, sockaddr_in6, dgram_sockaddr_ll>;

/*
 * Static tag for sanity checking random pointers are actually descriptors.
 * Can be removed once we're sure this works :)
 */
static constexpr uint64_t descriptor_tag = 0xABCCDDDEEEEE;

/* Each datagram in the buffer is pre-pended with the following descriptor */
struct dgram_channel_descriptor
{
    uint64_t tag = descriptor_tag;
    std::optional<dgram_channel_addr> address;
    uint16_t length;
};

/*
 * The datagram channel uses the same approach as the stream
 * channel.  But because we're dealing with packets of data,
 * we pre-pend each write to the buffer with a fixed size header.
 * This header allows the reader to determine the address for the
 * payload and the actual size of the data which follows.
 *
 * Additionally, just like the stream channel, we maintain two
 * individual buffers, one for transmit and one for receive,
 * and we used eventfd's to signal between client and server
 * when necessary.
 */

#define DGRAM_CHANNEL_MEMBERS                                                  \
    struct alignas(cache_line_size)                                            \
    {                                                                          \
        buffer tx_buffer;                                                      \
        api::socket_fd_pair client_fds;                                        \
        std::atomic_uint64_t tx_q_write_idx;                                   \
        std::atomic_uint64_t rx_q_read_idx;                                    \
        std::atomic_uint64_t tx_fd_write_idx;                                  \
        std::atomic_uint64_t rx_fd_read_idx;                                   \
        std::atomic_int socket_flags;                                          \
    };                                                                         \
    struct alignas(cache_line_size)                                            \
    {                                                                          \
        buffer rx_buffer;                                                      \
        api::socket_fd_pair server_fds;                                        \
        std::atomic_uint64_t tx_q_read_idx;                                    \
        std::atomic_uint64_t rx_q_write_idx;                                   \
        std::atomic_uint64_t tx_fd_read_idx;                                   \
        std::atomic_uint64_t rx_fd_write_idx;                                  \
        void* allocator;                                                       \
    };

struct dgram_channel
{
    DGRAM_CHANNEL_MEMBERS
};

} // namespace openperf::socket

#endif /* _OP_SOCKET_DGRAM_CHANNEL_HPP_ */
