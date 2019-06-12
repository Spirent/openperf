#include <cassert>

#include "socket/server/compat/linux/tcp.h"
#include "socket/server/socket_utils.h"
#include "socket/server/lwip_utils.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "packetio/stack/netif_utils.h"

namespace icp {
namespace socket {
namespace server {

tl::expected<socklen_t, int> do_sock_getsockopt(const ip_pcb* pcb,
                                                const api::request_getsockopt& getsockopt)
{
    assert(getsockopt.level == SOL_SOCKET);

    socklen_t slength = 0;
    switch (getsockopt.optname) {
    case SO_BROADCAST: {
        int opt = !!ip_get_option(pcb, SOF_BROADCAST);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, opt);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(opt);
        break;
    }
    case SO_KEEPALIVE: {
        int opt = !!ip_get_option(pcb, SOF_KEEPALIVE);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, opt);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(opt);
        break;
    }
    case SO_REUSEADDR:
    case SO_REUSEPORT: {
        int opt = !!ip_get_option(pcb, SOF_REUSEADDR);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, opt);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(opt);
        break;
    }
    case SO_SNDBUF:
    case SO_RCVBUF: {
        /* XXX: need to get this programatically... */
        int size = 48 * 1024;
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, size);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(size);
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return (slength);
}

tl::expected<void, int> do_sock_setsockopt(ip_pcb* pcb,
                                           const api::request_setsockopt& setsockopt)
{
    assert(setsockopt.level == SOL_SOCKET);

    switch (setsockopt.optname) {
    case SO_BROADCAST: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt) ip_set_option(pcb, SOF_BROADCAST);
        else      ip_reset_option(pcb, SOF_BROADCAST);
        break;
    }
    case SO_KEEPALIVE: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int *>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt) ip_set_option(pcb, SOF_KEEPALIVE);
        else      ip_reset_option(pcb, SOF_KEEPALIVE);
        break;
    }
    case SO_REUSEADDR:
    case SO_REUSEPORT: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt) ip_set_option(pcb, SOF_REUSEADDR);
        else      ip_reset_option(pcb, SOF_REUSEADDR);
        break;
    }
    case SO_BINDTODEVICE: {
        std::string optval(setsockopt.optlen, '\0');
        auto opt = copy_in(optval.data(),
                           setsockopt.id.pid,
                           reinterpret_cast<const char*>(setsockopt.optval),
                           setsockopt.optlen,
                           setsockopt.optlen);
        if (!opt) return (tl::make_unexpected(opt.error()));
        auto idx = netif_id_match(optval);
        if (idx == NETIF_NO_INDEX) {
            return (tl::make_unexpected(ENODEV));
        }
        pcb->netif_idx = idx;
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return {};
}

tl::expected<socklen_t, int> do_ip_getsockopt(const ip_pcb* pcb,
                                              const api::request_getsockopt& getsockopt)
{
    assert(getsockopt.level == IPPROTO_IP);

    socklen_t slength = 0;
    switch (getsockopt.optname) {
    case IP_TOS: {
        int tos = pcb->tos;
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, tos);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(tos);
        break;
    }
    case IP_TTL: {
        int ttl = pcb->ttl;
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, ttl);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(ttl);
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return (slength);
}

tl::expected<void, int> do_ip_setsockopt(ip_pcb* pcb,
                                         const api::request_setsockopt& setsockopt)
{
    assert(setsockopt.level == IPPROTO_IP);

    switch (setsockopt.optname) {
    case IP_TOS: {
        auto tos = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!tos) return (tl::make_unexpected(tos.error()));
        if (*tos < 0 || *tos > 255) return (tl::make_unexpected(EINVAL));
        pcb->tos = *tos;
        break;
    }
    case IP_TTL: {
        auto ttl = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!ttl) return (tl::make_unexpected(ttl.error()));
        if (*ttl < 0 || *ttl > 255) return (tl::make_unexpected(EINVAL));
        pcb->ttl = *ttl;
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return {};
}


static int lwip_to_linux_state(enum tcp_state state)
{
    switch (state) {
    case CLOSED: return (LINUX_TCP_CLOSE);
    case LISTEN: return (LINUX_TCP_LISTEN);
    case SYN_SENT: return (LINUX_TCP_SYN_SENT);
    case SYN_RCVD: return (LINUX_TCP_SYN_RECV);
    case ESTABLISHED: return (LINUX_TCP_ESTABLISHED);
    case FIN_WAIT_1: return (LINUX_TCP_FIN_WAIT1);
    case FIN_WAIT_2: return (LINUX_TCP_FIN_WAIT2);
    case CLOSE_WAIT: return (LINUX_TCP_CLOSE_WAIT);
    case CLOSING: return (LINUX_TCP_CLOSING);
    case LAST_ACK: return (LINUX_TCP_LAST_ACK);
    case TIME_WAIT: return (LINUX_TCP_TIME_WAIT);
    }
}

void get_tcp_info(const tcp_pcb* pcb, tcp_info& info)
{
    info.tcpi_state = lwip_to_linux_state(pcb->state);
    info.tcpi_retransmits = pcb->nrtx;

#if LWIP_WND_SCALE
    info.tcpi_snd_wscale = pcb->snd_scale;
    info.tcpi_rcv_wscale = pcb->rcv_scale;
#endif

    info.tcpi_rto = pcb->rto;
    info.tcpi_snd_mss = tcp_mss(pcb);
    info.tcpi_rcv_mss = tcp_mss(pcb);

    info.tcpi_unacked = (pcb->unacked ? pcb->unacked->len : 0);

#if LWIP_TCP_TIMESTAMPS
    info.tcpi_last_data_sent = pcb->ts_recent;
    info.tcpi_last_ack_sent = pcb->ts_lastacksent;
#endif

    info.tcpi_rtt = pcb->rttest * 500;
    info.tcpi_snd_ssthresh = pcb->ssthresh;
    info.tcpi_snd_cwnd = pcb->cwnd / tcp_mss(pcb);  /* in units of segments */

    info.tcpi_total_retrans = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(tcp_ext_arg_get(pcb, 0)));

    info.tcpi_notsent_bytes = (pcb->unsent ? pcb->unsent->len : 0);
}

}
}
}
