#ifndef _ICP_SOCKET_IO_CHANNEL_H_
#define _ICP_SOCKET_IO_CHANNEL_H_

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

struct io_ipv4_addr {
    uint32_t addr;
};

struct io_ipv6_addr {
    uint32_t addr[4];
};

/* Congruent to lwIP ip_addr_t; otherwise we'd use a variant */
struct io_ip_addr {
    enum class type:uint8_t {IPv4 = 0, IPv6 = 6};
    union {
        io_ipv6_addr ip6;
        io_ipv4_addr ip4;
    } u_addr;
    type type;
};

class io_channel_addr {
    io_ip_addr m_addr;
    in_port_t  m_port;

public:
    io_channel_addr()
        : m_addr({ { { 0, 0, 0, 0 } }, io_ip_addr::type::IPv4})
        , m_port(0)
    {}

    io_channel_addr(const io_ip_addr* addr, in_port_t port)
        : m_addr(*addr)
        , m_port(port)
    {}

    io_channel_addr(uint32_t addr, in_port_t port)
        : m_addr({ { { addr, 0, 0, 0 } }, io_ip_addr::type::IPv4})
        , m_port(port)
    {}

    io_channel_addr(const uint32_t addr[], in_port_t port)
        : m_addr({ { { addr[0], addr[1], addr[2], addr[3] } }, io_ip_addr::type::IPv6})
        , m_port(port)
    {}

    const io_ip_addr& addr() const { return (m_addr); }
    in_port_t port() const { return (m_port); }
};

class io_channel_buf
{
    const pbuf* m_pbuf;
    const void* m_payload;
    uint32_t m_length:31;
    uint32_t m_more:1;

public:
    io_channel_buf()
        : m_pbuf(nullptr)
        , m_payload(nullptr)
        , m_length(0)
        , m_more(false)
    {}

    io_channel_buf(const pbuf* pbuf, const void* payload, size_t length, bool more = false)
        : m_pbuf(pbuf)
        , m_payload(payload)
        , m_length(length)
        , m_more(more)
    {}

    io_channel_buf(const void* payload, size_t length, bool more = false)
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

typedef std::variant<io_channel_buf, io_channel_addr> io_channel_item;

inline bool more(const io_channel_item& item)
{
    return (std::visit(overloaded_visitor(
                           [](const io_channel_addr&) -> bool {
                               return (false);
                           },
                           [](const io_channel_buf& buf) -> bool {
                               return (buf.more());
                           }),
                       item));
}

#define IO_CHANNEL_MEMBERS                                      \
    spsc_queue<io_channel_item,   /* from stack to client */    \
               api::socket_queue_length> recvq;                 \
    spsc_queue<io_channel_item,   /* from client to stack */    \
               api::socket_queue_length> sendq;                 \
    spsc_queue<const pbuf*,       /* pbuf pointers to free */   \
               api::socket_queue_length> freeq;                 \
    api::socket_fd_pair client_fds;                             \
    api::socket_fd_pair server_fds;                             \
    int send_error;

struct io_channel {
    IO_CHANNEL_MEMBERS
};

}
}

#endif /* _ICP_SOCKET_IO_CHANNEL_H_ */
