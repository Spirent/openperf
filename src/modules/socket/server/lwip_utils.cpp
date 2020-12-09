#include <cassert>

#include <sys/ioctl.h>

#include "socket/server/compat/linux/if.h"
#include "socket/server/compat/linux/tcp.h"
#include "socket/server/socket_utils.hpp"
#include "socket/server/lwip_utils.hpp"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "packet/stack/lwip/netif_utils.hpp"
#include "timesync/chrono.hpp"

namespace openperf::socket::server {

static std::optional<ifreq> get_siogifconf_ifr(const netif* netif)
{
    auto* addr = netif_ip4_addr(netif);
    if (!addr) { return (std::nullopt); }

    auto id = packet::stack::interface_id_by_netif(netif);

    auto ifr = ifreq{};

    auto* sin = reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = addr->addr;

    std::copy_n(id.data(),
                std::min(static_cast<size_t>(IFNAMSIZ), id.length()),
                ifr.ifr_name);

    return (ifr);
}

static std::vector<ifreq> get_siocgifconf_ifrs()
{
    auto data = std::vector<ifreq>{};
    netif* ifp = nullptr;
    NETIF_FOREACH (ifp) {
        if (auto ifr = get_siogifconf_ifr(ifp)) { data.push_back(*ifr); }
    }
    return (data);
}

static std::vector<ifreq> get_siocgifconf_ifrs(uint8_t netif_idx)
{
    auto data = std::vector<ifreq>{};
    if (auto* netif = netif_get_by_index(netif_idx)) {
        if (auto ifr = get_siogifconf_ifr(netif)) { data.push_back(*ifr); }
    }
    return (data);
}

static uint16_t get_siocgifflags(const struct netif* netif)
{
    uint16_t flags = IFF_RUNNING;

    if (netif_is_flag_set(netif, NETIF_FLAG_UP)) { flags |= IFF_UP; }

    if (netif_is_flag_set(netif, NETIF_FLAG_BROADCAST)) {
        flags |= IFF_BROADCAST;
    }

    /*
     * This flag is defined in the Linux netdevice manual page, but not
     * in glibc, apparently.  Leave it out for now.
     */
    // if (netif_is_flag_set(netif, NETIF_FLAG_LINK_UP)) {
    //    flags |= IFF_LOWER_UP;
    //}

    if (!netif_is_flag_set(netif, NETIF_FLAG_ETHARP)) { flags |= IFF_NOARP; }

    if (netif_is_flag_set(netif, NETIF_FLAG_IGMP)
        || netif_is_flag_set(netif, NETIF_FLAG_MLD6)) {
        flags |= IFF_MULTICAST;
    }

    return (flags);
}

tl::expected<void, int> do_sock_ioctl(const ip_pcb* pcb,
                                      const api::request_ioctl& ioctl)
{
    switch (ioctl.request) {
    case SIOCGIFNAME: {
        auto ifr = ifreq{};
        auto result = copy_in(ifr, ioctl.id.pid, ioctl.argp);
        if (!result) { return (tl::make_unexpected(result.error())); }

        auto* netif = netif_get_by_index(ifr.ifr_ifindex);
        if (!netif) return (tl::make_unexpected(ENODEV));

        auto id = packet::stack::interface_id_by_netif(netif);
        std::copy_n(id.data(),
                    std::min(static_cast<size_t>(IFNAMSIZ), id.length()),
                    ifr.ifr_name);
        result = copy_out(ioctl.id.pid, ioctl.argp, ifr);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGIFINDEX: {
        auto ifr = ifreq{};
        auto result = copy_in(ifr, ioctl.id.pid, ioctl.argp);
        if (!result) { return (tl::make_unexpected(result.error())); }

        auto* netif = packet::stack::netif_get_by_name(ifr.ifr_name);
        if (!netif) return (tl::make_unexpected(ENODEV));

        ifr.ifr_ifindex = netif_get_index(netif);

        result = copy_out(ioctl.id.pid, ioctl.argp, ifr);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGIFFLAGS: {
        auto ifr = ifreq{};
        auto result = copy_in(ifr, ioctl.id.pid, ioctl.argp);
        if (!result) { return (tl::make_unexpected(result.error())); }

        auto* netif = packet::stack::netif_get_by_name(ifr.ifr_name);
        if (!netif) return (tl::make_unexpected(ENODEV));

        ifr.ifr_flags = get_siocgifflags(netif);

        result = copy_out(ioctl.id.pid, ioctl.argp, ifr);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGIFADDR: {
        auto ifr = ifreq{};
        auto result = copy_in(ifr, ioctl.id.pid, ioctl.argp);
        if (!result) { return (tl::make_unexpected(result.error())); }

        auto* netif = packet::stack::netif_get_by_name(ifr.ifr_name);
        if (!netif) return (tl::make_unexpected(ENODEV));

        auto* sin = reinterpret_cast<sockaddr_in*>(&ifr.ifr_addr);
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = netif_ip4_addr(netif)->addr;

        result = copy_out(ioctl.id.pid, ioctl.argp, ifr);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGIFMTU: {
        auto ifr = ifreq{};
        auto result = copy_in(ifr, ioctl.id.pid, ioctl.argp);
        if (!result) { return (tl::make_unexpected(result.error())); }

        auto* netif = packet::stack::netif_get_by_name(ifr.ifr_name);
        if (!netif) return (tl::make_unexpected(ENODEV));

        ifr.ifr_mtu = netif->mtu;

        result = copy_out(ioctl.id.pid, ioctl.argp, ifr);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGIFHWADDR: {
        auto ifr = ifreq{};
        auto result = copy_in(ifr, ioctl.id.pid, ioctl.argp);
        if (!result) { return (tl::make_unexpected(result.error())); }

        auto* netif = packet::stack::netif_get_by_name(ifr.ifr_name);
        if (!netif) return (tl::make_unexpected(ENODEV));

        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        std::copy_n(netif->hwaddr, netif->hwaddr_len, ifr.ifr_hwaddr.sa_data);

        result = copy_out(ioctl.id.pid, ioctl.argp, ifr);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGIFCONF: {
        auto conf = ifconf{};
        auto result = copy_in(conf, ioctl.id.pid, ioctl.argp);
        if (!result) return (tl::make_unexpected(result.error()));

        auto ifrs = (pcb->netif_idx == NETIF_NO_INDEX
                         ? get_siocgifconf_ifrs()
                         : get_siocgifconf_ifrs(pcb->netif_idx));

        if (conf.ifc_req == nullptr) {
            /* Caller only wants the buffer length */
            conf.ifc_len = ifrs.size() * sizeof(struct ifreq);
        } else {
            /* Caller wants as much data as will fit */
            auto to_copy =
                sizeof(ifreq)
                * std::min(ifrs.size(), (conf.ifc_len / sizeof(ifreq)));
            result = copy_out(ioctl.id.pid, conf.ifc_buf, ifrs.data(), to_copy);
            if (!result) return (tl::make_unexpected(result.error()));

            /* Update initial request with buffer length */
            conf.ifc_len = to_copy;
        }
        result = copy_out(ioctl.id.pid, ioctl.argp, conf);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    case SIOCGSTAMP: {
        /*
         * XXX: Should add the timestamp to the channel and let the
         * client retrieve it without having to call through the
         * domain socket.
         */
        using clock = openperf::timesync::chrono::realtime;
        auto now = timesync::to_timeval(clock::now().time_since_epoch());

        auto result = copy_out(ioctl.id.pid, ioctl.argp, now);
        if (!result) return (tl::make_unexpected(result.error()));
        break;
    }
    default:
        return (tl::make_unexpected(EINVAL));
    }

    return {};
}

tl::expected<socklen_t, int>
do_sock_getsockopt(const ip_pcb* pcb, const api::request_getsockopt& getsockopt)
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
    case SO_ERROR: {
        int error = 0;
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, error);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(error);
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return (slength);
}

tl::expected<void, int>
do_sock_setsockopt(ip_pcb* pcb, const api::request_setsockopt& setsockopt)
{
    assert(setsockopt.level == SOL_SOCKET);

    switch (setsockopt.optname) {
    case SO_BROADCAST: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt)
            ip_set_option(pcb, SOF_BROADCAST);
        else
            ip_reset_option(pcb, SOF_BROADCAST);
        break;
    }
    case SO_KEEPALIVE: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt)
            ip_set_option(pcb, SOF_KEEPALIVE);
        else
            ip_reset_option(pcb, SOF_KEEPALIVE);
        break;
    }
    case SO_REUSEADDR:
    case SO_REUSEPORT: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt)
            ip_set_option(pcb, SOF_REUSEADDR);
        else
            ip_reset_option(pcb, SOF_REUSEADDR);
        break;
    }
    case SO_BINDTODEVICE: {
        /*
         * Make sure our incoming string data is NULL terminated by ensuring we
         * have an extra char at the end.
         */
        auto optval = std::vector<char>(setsockopt.optlen + 1);
        auto opt = copy_in(optval.data(),
                           setsockopt.id.pid,
                           reinterpret_cast<const char*>(setsockopt.optval),
                           setsockopt.optlen,
                           setsockopt.optlen);
        if (!opt) return (tl::make_unexpected(opt.error()));
        auto idx = packet::stack::netif_index_by_name(optval.data());
        if (idx == NETIF_NO_INDEX) { return (tl::make_unexpected(ENODEV)); }
        pcb->netif_idx = idx;
        break;
    }
    case SO_RCVBUF:
        /* XXX: just ignore this request; our buffers are fixed */
        break;
    case SO_DETACH_FILTER:
        return (tl::make_unexpected(ENOENT));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return {};
}

tl::expected<socklen_t, int>
do_ip_getsockopt(const ip_pcb* pcb, const api::request_getsockopt& getsockopt)
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

tl::expected<void, int>
do_ip_setsockopt(ip_pcb* pcb, const api::request_setsockopt& setsockopt)
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

tl::expected<socklen_t, int>
do_ip6_getsockopt(const ip_pcb* pcb, const api::request_getsockopt& getsockopt)
{
    assert(getsockopt.level == IPPROTO_IPV6);

    socklen_t slength = 0;
    switch (getsockopt.optname) {
    case IPV6_TCLASS: {
        int tos = pcb->tos;
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, tos);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(tos);
        break;
    }
    case IPV6_HOPLIMIT: {
        int ttl = pcb->ttl;
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, ttl);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(ttl);
        break;
    }
    case IPV6_RECVHOPLIMIT: {
        // FIXME: Need this for ping6 to print out ttl values
        break;
    }
    case IPV6_V6ONLY: {
        int v6only = IP_IS_V6_VAL(pcb->local_ip);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, v6only);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(v6only);
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return (slength);
}

tl::expected<void, int>
do_ip6_setsockopt(ip_pcb* pcb, const api::request_setsockopt& setsockopt)
{
    assert(setsockopt.level == IPPROTO_IPV6);

    switch (setsockopt.optname) {
    case IPV6_TCLASS: {
        auto tos = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!tos) return (tl::make_unexpected(tos.error()));
        if (*tos < 0 || *tos > 255) return (tl::make_unexpected(EINVAL));
        pcb->tos = *tos;
        break;
    }
    case IPV6_HOPLIMIT: {
        auto ttl = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!ttl) return (tl::make_unexpected(ttl.error()));
        if (*ttl < 0 || *ttl > 255) return (tl::make_unexpected(EINVAL));
        pcb->ttl = *ttl;
        break;
    }
    case IPV6_RECVERR:
    case IPV6_RECVHOPLIMIT:
        /*
         * FIXME: The ping6 binary needs both of these to function; we don't
         * actually support them, but we let clients think we do.
         */
        break;
    case IPV6_V6ONLY: {
        auto v6only = copy_in(setsockopt.id.pid,
                              reinterpret_cast<const int*>(setsockopt.optval));
        if (!v6only) return (tl::make_unexpected(v6only.error()));
        if (*v6only) {
            if (!IP_IS_V6_VAL(pcb->local_ip)) {
                if (!IP_IS_ANY_TYPE_VAL(pcb->local_ip))
                    return (tl::make_unexpected(EINVAL));
                IP_SET_TYPE_VAL(pcb->local_ip, IPADDR_TYPE_V6);
                IP_SET_TYPE_VAL(pcb->remote_ip, IPADDR_TYPE_V6);
            }
        } else {
            if (!IP_IS_ANY_TYPE_VAL(pcb->local_ip)) {
                if (!IP_IS_V6_VAL(pcb->local_ip))
                    return (tl::make_unexpected(EINVAL));
                IP_SET_TYPE_VAL(pcb->local_ip, IPADDR_TYPE_ANY);
                IP_SET_TYPE_VAL(pcb->remote_ip, IPADDR_TYPE_ANY);
            }
        }
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
    case CLOSED:
        return (LINUX_TCP_CLOSE);
    case LISTEN:
        return (LINUX_TCP_LISTEN);
    case SYN_SENT:
        return (LINUX_TCP_SYN_SENT);
    case SYN_RCVD:
        return (LINUX_TCP_SYN_RECV);
    case ESTABLISHED:
        return (LINUX_TCP_ESTABLISHED);
    case FIN_WAIT_1:
        return (LINUX_TCP_FIN_WAIT1);
    case FIN_WAIT_2:
        return (LINUX_TCP_FIN_WAIT2);
    case CLOSE_WAIT:
        return (LINUX_TCP_CLOSE_WAIT);
    case CLOSING:
        return (LINUX_TCP_CLOSING);
    case LAST_ACK:
        return (LINUX_TCP_LAST_ACK);
    case TIME_WAIT:
        return (LINUX_TCP_TIME_WAIT);
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
    info.tcpi_snd_cwnd = pcb->cwnd / tcp_mss(pcb); /* in units of segments */

    info.tcpi_total_retrans = static_cast<uint32_t>(
        reinterpret_cast<uintptr_t>(tcp_ext_arg_get(pcb, 0)));

    info.tcpi_notsent_bytes = (pcb->unsent ? pcb->unsent->len : 0);
}

} // namespace openperf::socket::server
