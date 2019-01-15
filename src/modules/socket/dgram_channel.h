#ifndef _ICP_SOCKET_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_DGRAM_CHANNEL_H_

#include <netinet/in.h>
#include <socket/bipartite_ring.h>
#include <socket/api.h>

struct pbuf;

namespace icp {
namespace socket {

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

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
    in_port_t  m_port;

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

class dgram_channel_data
{
    /*
     * We store the length in the upper byte of the pbuf and payload pointers.
     * When machines use more than 48 bits for addressing we can update this. :)
     */
    static constexpr uintptr_t ptr_mask = 0xffffffffffff;
    static constexpr int ptr_shift = 56;
    pbuf* m_pbuf;
    void* m_payload;

public:
    dgram_channel_data() {};

    dgram_channel_data(pbuf* pbuf, void* payload, uint16_t len)
        : m_pbuf(pbuf)
        , m_payload(payload)
    {
        length(len);
    }

    pbuf* pbuf() { return (reinterpret_cast<struct pbuf*>(reinterpret_cast<uintptr_t>(m_pbuf) & ptr_mask)); }
    void* payload() { return (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_payload) & ptr_mask)); }
    uint16_t length() { return (static_cast<uint16_t>(
                                    (reinterpret_cast<uintptr_t>(m_pbuf) >> ptr_shift) << 8
                                    | (reinterpret_cast<uintptr_t>(m_payload) >> ptr_shift))); }

    void length(uint16_t length)
    {
        m_pbuf = reinterpret_cast<struct pbuf*>(((static_cast<uint64_t>(length) >> 8) << ptr_shift)
                                                | (reinterpret_cast<uintptr_t>(m_pbuf) & ptr_mask));
        m_payload = reinterpret_cast<void*>(((static_cast<uint64_t>(length) & 0xff) << ptr_shift)
                                            | (reinterpret_cast<uintptr_t>(m_payload) & ptr_mask));
    }

};

struct dgram_channel_item {
    std::optional<dgram_channel_addr> address;
    dgram_channel_data data;
};

typedef bipartite_ring<dgram_channel_item, api::socket_queue_length> dgram_ring;

#define DGRAM_CHANNEL_MEMBERS                                       \
    dgram_ring recvq;  /* from stack to client */                   \
    dgram_ring sendq;  /* from client to stack */                   \
    api::socket_fd_pair client_fds;                                 \
    api::socket_fd_pair server_fds;

struct dgram_channel {
    DGRAM_CHANNEL_MEMBERS
};

}
}

#endif /* _ICP_SOCKET_DGRAM_CHANNEL_H_ */
