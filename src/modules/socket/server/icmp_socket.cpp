#include <cassert>
#include <cstring>
#include <numeric>
#include <optional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "socket/server/icmp_socket.h"
#include "socket/server/lwip_utils.h"
#include "socket/server/raw_socket.h"
#include "lwip/memp.h"
#include "lwip/raw.h"


namespace icp {
namespace socket {
namespace server {

uint8_t icmp_receive(void* arg, raw_pcb* pcb, pbuf* p, const ip_addr_t* addr)
{
    (void)pcb;
    assert(addr != nullptr);
    auto sock = reinterpret_cast<icmp_socket*>(arg);
    auto icmp_filter = sock->icmp_filter();

    if (icmp_filter != 0) {
        auto type = *((uint8_t *)p->payload);

        switch (type) {
        case LINUX_ICMP_ECHOREPLY:
        case LINUX_ICMP_DEST_UNREACH:
        case LINUX_ICMP_SOURCE_QUENCH:
        case LINUX_ICMP_REDIRECT:
        case LINUX_ICMP_TIME_EXCEEDED:
        case LINUX_ICMP_PARAMETERPROB:
            if (icmp_filter & (1 << type)) {
                pbuf_free(p);
                return (1);
            }
        default:
            break;
        }
    }

    auto channel = std::get<dgram_channel*>(sock->channel());
    if (!channel->send(p, reinterpret_cast<const dgram_ip_addr*>(addr), 0)) {
        return (0);
    }
    return (1);
}

icmp_socket::icmp_socket(icp::socket::server::allocator& allocator, int flags, int protocol)
    : raw_socket(allocator, flags, protocol, &icmp_receive)
    , m_icmp_filter(0)
{
}

tl::expected<socklen_t, int> icmp_socket::do_getsockopt(const raw_pcb* pcb,
                                                        const api::request_getsockopt& getsockopt)
{
    switch (getsockopt.level) {
    case SOL_RAW:
        switch (getsockopt.optname) {
        case LINUX_ICMP_FILTER: {
            auto slength = getsockopt.optlen;
            if (pcb->protocol != IPPROTO_ICMP) return (tl::make_unexpected(EOPNOTSUPP));
            if (slength > sizeof(sizeof m_icmp_filter)) slength = sizeof(m_icmp_filter);
            auto result = copy_out(getsockopt.id.pid,
                                   getsockopt.optval,
                                   &m_icmp_filter,
                                   slength);
            if (!result) return (tl::make_unexpected(result.error()));
            return (slength);
        }
        default:
            return (tl::make_unexpected(ENOPROTOOPT));
        }
    case SOL_SOCKET:
        return (do_sock_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    case IPPROTO_IP:
        return (do_ip_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tl::expected<void, int> icmp_socket::do_setsockopt(raw_pcb* pcb,
                                                   const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.level) {
    case SOL_RAW:
        switch (setsockopt.optname) {
        case LINUX_ICMP_FILTER: {
            if (pcb->protocol != IPPROTO_ICMP) return (tl::make_unexpected(EOPNOTSUPP));
            auto opt = copy_in(setsockopt.id.pid,
                reinterpret_cast<const uint32_t *>(setsockopt.optval));
            if (!opt) return (tl::make_unexpected(opt.error()));
            m_icmp_filter = *opt;
            return {};
        }
        default:
            return (tl::make_unexpected(ENOPROTOOPT));
        }
    case SOL_SOCKET:
        return (do_sock_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_IP:
        return (do_ip_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

uint32_t icmp_socket::icmp_filter()
{
    return (m_icmp_filter);
}

}
}
}
