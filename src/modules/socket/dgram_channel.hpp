#ifndef _OP_SOCKET_DGRAM_CHANNEL_HPP_
#define _OP_SOCKET_DGRAM_CHANNEL_HPP_

#include <atomic>

#include <netinet/in.h>

#include "framework/memory/offset_ptr.hpp"
#include "socket/api.hpp"

namespace openperf {
namespace socket {

struct dgram_ipv4_addr {
    uint32_t addr;
};

struct dgram_ipv6_addr {
    uint32_t addr[4];
};

/* Congruent to lwIP ip_addr_t; otherwise we'd use a variant */
struct dgram_ip_addr {
    enum class type:uint8_t {IPv4 = 0, IPv6 = 6};
    union {
        dgram_ipv6_addr ip6;
        dgram_ipv4_addr ip4;
    } u_addr;
    type type;
};

class dgram_channel_addr {
    dgram_ip_addr m_addr;
    in_port_t m_port;

public:
    dgram_channel_addr()
        : m_addr({ { { 0, 0, 0, 0 } }, dgram_ip_addr::type::IPv4})
        , m_port(0)
    {}

    dgram_channel_addr(const dgram_ip_addr* addr, in_port_t port)
        : m_addr(*addr)
        , m_port(port)
    {}

    dgram_channel_addr(uint32_t addr, in_port_t port)
        : m_addr({ { { addr, 0, 0, 0 } }, dgram_ip_addr::type::IPv4})
        , m_port(port)
    {}

    dgram_channel_addr(const uint32_t addr[], in_port_t port)
        : m_addr({ { { addr[0], addr[1], addr[2], addr[3] } }, dgram_ip_addr::type::IPv6})
        , m_port(port)
    {}

    const dgram_ip_addr& addr() const { return (m_addr); }
    in_port_t port() const { return (m_port); }
};

/*
 * Static tag for sanity checking random pointers are actually descriptors.
 * Can be removed once we're sure this works :)
 */
static constexpr uint64_t descriptor_tag = 0xABCCDDDEEEEE;

/* Each datagram in the buffer is pre-pended with the following descriptor */
struct dgram_channel_descriptor {
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

#define DGRAM_CHANNEL_MEMBERS                         \
    struct alignas(cache_line_size) {                 \
        buffer tx_buffer;                             \
        api::socket_fd_pair client_fds;               \
        std::atomic_uint64_t tx_q_write_idx;          \
        std::atomic_uint64_t rx_q_read_idx;           \
        std::atomic_uint64_t tx_fd_write_idx;         \
        std::atomic_uint64_t rx_fd_read_idx;          \
        std::atomic_int socket_flags;                 \
    };                                                \
    struct alignas(cache_line_size) {                 \
        buffer rx_buffer;                             \
        api::socket_fd_pair server_fds;               \
        std::atomic_uint64_t tx_q_read_idx;           \
        std::atomic_uint64_t rx_q_write_idx;          \
        std::atomic_uint64_t tx_fd_read_idx;          \
        std::atomic_uint64_t rx_fd_write_idx;         \
        void* allocator;                              \
    };

struct dgram_channel {
    DGRAM_CHANNEL_MEMBERS
};

}
}

#endif /* _OP_SOCKET_DGRAM_CHANNEL_HPP_ */
