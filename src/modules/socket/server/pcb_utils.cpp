#include <cassert>
#include <cerrno>
#include <system_error>
#include <algorithm>
#include <unordered_map>
#include <arpa/inet.h>

#include "socket/api.hpp"
#include "socket/server/generic_socket.hpp"
#include "socket/server/icmp_socket.hpp"
#include "socket/server/raw_socket.hpp"
#include "socket/server/tcp_socket.hpp"
#include "socket/server/udp_socket.hpp"
#include "socket/server/socket_utils.hpp"
#include "socket/server/pcb_utils.hpp"
#include "socket/server/api_server.hpp"
#include "core/op_log.h"
#include "utils/overloaded_visitor.hpp"

#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "packet/stack/lwip/packet.h"

namespace openperf::socket::server {

static void foreach_socket(
    std::function<void(const api::socket_id& id,
                       const socket::server::generic_socket&)>&& func)
{
    auto svr = openperf::socket::api::get_socket_server();
    if (!svr) return;
    svr->foreach_socket(std::move(func));
}

static channel_stats
get_channel_stats(const socket::server::channel_variant& channel)
{
    auto stats_visitor = [](auto channel_ptr) -> channel_stats {
        return channel_stats{.txq_len = channel_ptr->readable(),
                             .rxq_len = channel_ptr->send_consumable()};
    };
    return (std::visit(stats_visitor, channel));
}

static channel_stats
get_socket_channel_stats(const socket::server::generic_socket& socket)
{
    return get_channel_stats(socket.channel());
}

static int ip_addr_type_to_family(uint8_t type)
{
    if (type == IPADDR_TYPE_V4) { return AF_INET; }
    if (type == IPADDR_TYPE_V6) { return AF_INET6; }
    return AF_INET6;
}

static size_t get_addr_length(uint8_t af)
{
    if (af == AF_INET) return 4;
    return 16;
}

template <typename Dest, typename Src> void copy_ip_pcb(Dest& d, const Src& s)
{
    d.if_index = s.netif_idx;
    d.ip_address_family = ip_addr_type_to_family(s.local_ip.type);
    auto len = get_addr_length(d.ip_address_family);
    memcpy(d.local_ip.data(), (void*)&s.local_ip.u_addr, len);
    memcpy(d.remote_ip.data(), (void*)&s.remote_ip.u_addr, len);
}

ip_pcb_stats get_ip_pcb_stats(const ip_pcb* pcb)
{
    ip_pcb_stats ip{};
    copy_ip_pcb(ip, *pcb);
    return ip;
}

void foreach_tcp_pcb(std::function<void(const tcp_pcb*)>&& pcb_func)
{
    std::for_each(tcp_pcb_lists,
                  tcp_pcb_lists + LWIP_ARRAYSIZE(tcp_pcb_lists),
                  [&](auto& pcb_list) {
                      auto pcb = *pcb_list;
                      while (pcb != nullptr) {
                          pcb_func(pcb);
                          pcb = pcb->next;
                      }
                  });
}

tcp_pcb_stats get_tcp_pcb_stats(const tcp_pcb* pcb)
{
    tcp_pcb_stats tcp{};
    copy_ip_pcb(tcp, *pcb);
    tcp.local_port = pcb->local_port;
    tcp.remote_port = pcb->remote_port;
    tcp.state = pcb->state;
    tcp.snd_queuelen = pcb->snd_queuelen;
    return tcp;
}

void foreach_udp_pcb(std::function<void(const udp_pcb*)>&& pcb_func)
{
    auto pcb = udp_pcbs;
    while (pcb != nullptr) {
        pcb_func(pcb);
        pcb = pcb->next;
    }
}

udp_pcb_stats get_udp_pcb_stats(const udp_pcb* pcb)
{
    udp_pcb_stats udp{};
    copy_ip_pcb(udp, *pcb);
    udp.local_port = pcb->local_port;
    udp.remote_port = pcb->remote_port;
    return udp;
}

raw_pcb_stats get_raw_pcb_stats(const raw_pcb* pcb)
{
    raw_pcb_stats raw{};
    copy_ip_pcb(raw, *pcb);
    raw.protocol = pcb->protocol;
    return raw;
}

packet_pcb_stats get_packet_pcb_stats(const packet_pcb* pcb)
{
    packet_pcb_stats packet{};
    packet.if_index = packet_get_ifindex(pcb);
    packet.packet_type = packet_get_packet_type(pcb);
    packet.protocol = packet_get_proto(pcb);
    return packet;
}

static pcb_stats to_pcb_stats(const pcb_variant& pcb)
{
    return std::visit(
        utils::overloaded_visitor(
            [&](const ip_pcb* ptr) { return pcb_stats{get_ip_pcb_stats(ptr)}; },
            [&](const tcp_pcb* ptr) {
                return pcb_stats{get_tcp_pcb_stats(ptr)};
            },
            [&](const udp_pcb* ptr) {
                return pcb_stats{get_udp_pcb_stats(ptr)};
            },
            [&](const raw_pcb* ptr) {
                return pcb_stats{get_raw_pcb_stats(ptr)};
            },
            [&](const packet_pcb* ptr) {
                return pcb_stats{get_packet_pcb_stats(ptr)};
            }),
        pcb);
}

std::vector<socket_pcb_stats>
get_matching_socket_pcb_stats(std::function<bool(const void*)>&& pcb_match_func)
{
    std::unordered_map<const void*, socket_pcb_stats> stats_map;

    /* Add PCB stats from LWIP known PCBs to the list.
     * There could be PCBs found here which do not have sockets
     * (e.g. sockets which have been closed)
     */
    foreach_tcp_pcb([&](auto t_pcb) {
        auto pcb = static_cast<const void*>(t_pcb);
        if (pcb_match_func && pcb_match_func(pcb)) {
            stats_map.emplace(
                pcb,
                socket_pcb_stats{.pcb = pcb,
                                 .pcb_stats = get_tcp_pcb_stats(t_pcb)});
        }
    });
    foreach_udp_pcb([&](auto u_pcb) {
        auto pcb = static_cast<const void*>(u_pcb);
        if (pcb_match_func && pcb_match_func(pcb)) {
            stats_map.emplace(
                pcb,
                socket_pcb_stats{.pcb = pcb,
                                 .pcb_stats = get_udp_pcb_stats(u_pcb)});
        }
    });

    /* Update PCB stats w/ info from sockets.
     * There shouldn't be any TCP/UDP sockets w/ PCBs which are not known by
     * LWIP, but there could be RAW, IP or packet sockets.
     */
    foreach_socket([&](auto& api_id, auto& sock) {
        socket_id id{.pid = api_id.pid, .sid = api_id.sid};
        auto pcb = std::visit(
            [](auto obj) { return reinterpret_cast<const void*>(obj); },
            sock.pcb());
        if (pcb_match_func && !pcb_match_func(pcb)) { return; }

        if (pcb) {
            auto found = stats_map.find(pcb);
            if (found != stats_map.end()) {
                // Update existing entry
                (found->second).channel_stats = get_socket_channel_stats(sock);
                (found->second).id = id;
            } else {
                // Socket with PCB but not in PCBs disovered by scanning lwip
                // lists.   Only TCP and UDP lwip PCB lists are scanned so other
                // sockets will get added here.
                //
                // There is currently no need to scan lwip PCB lists for other
                // sockets because they don't have complex state like TCP.
                // i.e. they should go away when the socket is closed.
                stats_map.emplace(
                    pcb,
                    socket_pcb_stats{.pcb = pcb,
                                     .pcb_stats = to_pcb_stats(sock.pcb()),
                                     .id = id,
                                     .channel_stats =
                                         get_socket_channel_stats(sock)});
            }
        } else {
            // Socket without a PCB (shouldn't happen)
            // protocol will show up as "raw" and there will be no stats
            // because there is no PCB.
            stats_map.emplace(
                pcb,
                socket_pcb_stats{.pcb = pcb,
                                 .id = id,
                                 .channel_stats =
                                     get_socket_channel_stats(sock)});
            OP_LOG(OP_LOG_WARNING, "Found socket with no pcb");
        }
    });

    std::vector<socket_pcb_stats> stats_list;
    stats_list.reserve(stats_map.size());
    std::transform(stats_map.begin(),
                   stats_map.end(),
                   std::back_inserter(stats_list),
                   [](auto it) { return it.second; });
    return stats_list;
}

std::vector<socket_pcb_stats> get_all_socket_pcb_stats()
{
    return get_matching_socket_pcb_stats({});
}

void dump_socket_pcb_stats()
{
    auto stats_list = get_all_socket_pcb_stats();

    std::for_each(stats_list.begin(), stats_list.end(), [](auto& stats) {
        char ip_buf[INET_ADDRSTRLEN];
        std::visit(
            utils::overloaded_visitor(
                [&](const std::monostate&) {
                    const char* proto = "raw";
                    fprintf(stderr,
                            "protocol=%s pid=%ld sid=%d "
                            "txq_len=%" PRIu64 " rxq_len=%" PRIu64 "\n",
                            proto,
                            (long)stats.id.pid,
                            stats.id.sid,
                            stats.channel_stats.txq_len,
                            stats.channel_stats.rxq_len);
                },
                [&](const ip_pcb_stats& pcbs) {
                    const char* proto = "ip";
                    std::string local_ip, remote_ip;
                    local_ip = inet_ntop(pcbs.ip_address_family,
                                         pcbs.local_ip.data(),
                                         ip_buf,
                                         sizeof(ip_buf))
                                   ? ip_buf
                                   : "<?>";
                    remote_ip = inet_ntop(pcbs.ip_address_family,
                                          pcbs.remote_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                    fprintf(stderr,
                            "protocol=%s if_index=%d local_ip=%s remote_ip=%s "
                            "pid=%ld sid=%d "
                            "txq_len=%" PRIu64 " rxq_len=%" PRIu64 "\n",
                            proto,
                            pcbs.if_index,
                            local_ip.c_str(),
                            remote_ip.c_str(),
                            (long)stats.id.pid,
                            stats.id.sid,
                            stats.channel_stats.txq_len,
                            stats.channel_stats.rxq_len);
                },
                [&](const raw_pcb_stats& pcbs) {
                    const char* proto = "raw";
                    std::string local_ip, remote_ip;
                    local_ip = inet_ntop(pcbs.ip_address_family,
                                         pcbs.local_ip.data(),
                                         ip_buf,
                                         sizeof(ip_buf))
                                   ? ip_buf
                                   : "<?>";
                    remote_ip = inet_ntop(pcbs.ip_address_family,
                                          pcbs.remote_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                    fprintf(stderr,
                            "protocol=%s if_index=%d local_ip=%s remote_ip=%s "
                            "ip_proto=%d pid=%ld sid=%d "
                            "txq_len=%" PRIu64 " rxq_len=%" PRIu64 "\n",
                            proto,
                            pcbs.if_index,
                            local_ip.c_str(),
                            remote_ip.c_str(),
                            (int)pcbs.protocol,
                            (long)stats.id.pid,
                            stats.id.sid,
                            stats.channel_stats.txq_len,
                            stats.channel_stats.rxq_len);
                },
                [&](const packet_pcb_stats& pcbs) {
                    const char* proto = "packet";
                    fprintf(
                        stderr,
                        "protocol=%s if_index=%d packet_type=%d protocol_id=%d "
                        "pid=%ld sid=%d "
                        "txq_len=%" PRIu64 " rxq_len=%" PRIu64 "\n",
                        proto,
                        pcbs.if_index,
                        pcbs.packet_type,
                        (int)pcbs.protocol,
                        (long)stats.id.pid,
                        stats.id.sid,
                        stats.channel_stats.txq_len,
                        stats.channel_stats.rxq_len);
                },
                [&](const tcp_pcb_stats& pcbs) {
                    const char* proto = "tcp";
                    std::string local_ip, remote_ip;
                    local_ip = inet_ntop(pcbs.ip_address_family,
                                         pcbs.local_ip.data(),
                                         ip_buf,
                                         sizeof(ip_buf))
                                   ? ip_buf
                                   : "<?>";
                    remote_ip = inet_ntop(pcbs.ip_address_family,
                                          pcbs.remote_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                    fprintf(
                        stderr,
                        "protocol=%s if_index=%d local_ip=%s remote_ip=%s "
                        "local_port=%" PRIu16 " remote_port=%" PRIu16 " "
                        "state=%s pid=%ld sid=%d "
                        "txq_len=%" PRIu64 " rxq_len=%" PRIu64 " "
                        "snd_queuelen=%" PRIu16 "\n",
                        proto,
                        pcbs.if_index,
                        local_ip.c_str(),
                        remote_ip.c_str(),
                        pcbs.local_port,
                        pcbs.remote_port,
                        tcp_debug_state_str(static_cast<tcp_state>(pcbs.state)),
                        (long)stats.id.pid,
                        stats.id.sid,
                        stats.channel_stats.txq_len,
                        stats.channel_stats.rxq_len,
                        pcbs.snd_queuelen);
                },
                [&](const udp_pcb_stats& pcbs) {
                    const char* proto = "udp";
                    std::string local_ip, remote_ip;
                    local_ip = inet_ntop(pcbs.ip_address_family,
                                         pcbs.local_ip.data(),
                                         ip_buf,
                                         sizeof(ip_buf))
                                   ? ip_buf
                                   : "<?>";
                    remote_ip = inet_ntop(pcbs.ip_address_family,
                                          pcbs.remote_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                    fprintf(stderr,
                            "protocol=%s if_index=%d local_ip=%s remote_ip=%s "
                            "local_port=%" PRIu16 " remote_port=%" PRIu16 " "
                            "pid=%ld sid=%d "
                            "txq_len=%" PRIu64 " rxq_len=%" PRIu64 "\n",
                            proto,
                            pcbs.if_index,
                            local_ip.c_str(),
                            remote_ip.c_str(),
                            pcbs.local_port,
                            pcbs.remote_port,
                            (long)stats.id.pid,
                            stats.id.sid,
                            stats.channel_stats.txq_len,
                            stats.channel_stats.rxq_len);
                }),
            stats.pcb_stats);
    });
}

} // namespace openperf::socket::server