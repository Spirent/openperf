#ifndef _ICP_SOCKET_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_DGRAM_CHANNEL_H_

#include <netinet/in.h>
#include <socket/spsc_queue.h>
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

class dgram_channel_buf
{
    const pbuf* m_pbuf;
    const void* m_payload;
    uint32_t m_length:31;
    uint32_t m_more:1;

public:
    dgram_channel_buf()
        : m_pbuf(nullptr)
        , m_payload(nullptr)
        , m_length(0)
        , m_more(false)
    {}

    dgram_channel_buf(const pbuf* pbuf, const void* payload, size_t length, bool more = false)
        : m_pbuf(pbuf)
        , m_payload(payload)
        , m_length(length)
        , m_more(more)
    {}

    dgram_channel_buf(const void* payload, size_t length, bool more = false)
        : m_pbuf(nullptr)
        , m_payload(payload)
        , m_length(length)
        , m_more(more)
    {}

    const pbuf* pbuf() const { return (m_pbuf); }
    const void* payload() const { return (m_payload); }
    size_t length() const { return (m_length); }
    bool more() const { return (m_more); }
};

typedef std::variant<dgram_channel_buf, dgram_channel_addr> dgram_channel_item;

inline bool more(const dgram_channel_item& item)
{
    return (std::visit(overloaded_visitor(
                           [](const dgram_channel_addr&) -> bool {
                               return (false);
                           },
                           [](const dgram_channel_buf& buf) -> bool {
                               return (buf.more());
                           }),
                       item));
}

#define DGRAM_CHANNEL_MEMBERS                                      \
    spsc_queue<dgram_channel_item,   /* from stack to client */    \
               api::socket_queue_length> recvq;                    \
    spsc_queue<dgram_channel_item,   /* from client to stack */    \
               api::socket_queue_length> sendq;                    \
    spsc_queue<const pbuf*,          /* pbuf pointers to free */   \
               api::socket_queue_length> freeq;                    \
    api::socket_fd_pair client_fds;                                \
    api::socket_fd_pair server_fds;

struct dgram_channel {
    DGRAM_CHANNEL_MEMBERS
};

}
}

#endif /* _ICP_SOCKET_DGRAM_CHANNEL_H_ */
