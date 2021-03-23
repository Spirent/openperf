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

namespace openperf::socket::server {

static void
foreach_socket(std::function<void(const api::socket_id& id,
                                  const socket::server::generic_socket&)> func)
{
    auto svr = openperf::socket::api::get_socket_server();
    if (!svr) return;
    svr->foreach_socket(func);
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

static int get_addr_length(uint8_t af)
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

void foreach_tcp_pcb(std::function<void(const tcp_pcb*)> pcb_func)
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
    // tcp.state = lwip_to_linux_state(pcb->state);
    tcp.state = pcb->state;
    tcp.snd_queuelen = pcb->snd_queuelen;
    return tcp;
}

void foreach_udp_pcb(std::function<void(const udp_pcb*)> pcb_func)
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

std::list<socket_pcb_stats> get_all_socket_pcb_stats()
{
    std::list<socket_pcb_stats> stats_list;
    std::unordered_map<const void*, socket_pcb_stats*> stats_map;

    /* Add PCB stats from LWIP known PCBs to the list.
     * There could be PCBs found here which do not have sockets
     * (e.g. sockets which have been closed)
     */
    foreach_tcp_pcb([&](auto pcb) {
        socket_pcb_stats stats{.pcb = static_cast<const void*>(pcb),
                               .pcb_stats = get_tcp_pcb_stats(pcb)};
        stats_list.push_back(stats);
        stats_map[stats.pcb] = &stats_list.back();
    });
    foreach_udp_pcb([&](auto pcb) {
        socket_pcb_stats stats{.pcb = static_cast<const void*>(pcb),
                               .pcb_stats = get_udp_pcb_stats(pcb)};
        stats_list.push_back(stats);
        stats_map[stats.pcb] = &stats_list.back();
    });

    /* Update PCB stats w/ info from sockets.
     * There shouldn't be any TCP/UDP sockets w/ PCBs which are not known by
     * LWIP, but there could be RAW sockets which don't support PCB stats yet.
     */
    foreach_socket([&](auto& api_id, auto& sock) {
        socket_id id{.pid = api_id.pid, .sid = api_id.sid};
        auto pcb = sock.pcb();
        if (pcb) {
            auto found = stats_map.find(pcb);
            if (found != stats_map.end()) {
                // Update existing entry
                (found->second)->channel_stats = get_socket_channel_stats(sock);
                (found->second)->id = id;
            } else {
                // Socket with PCB but with unknown protocol PCB type.
                // protocol will show up as "raw" and there will be no stats
                // because PCB type is not known.
                stats_list.push_back(socket_pcb_stats{
                    .pcb = pcb,
                    .id = id,
                    .channel_stats = get_socket_channel_stats(sock)});
                stats_map[pcb] = &stats_list.back();
            }
        } else {
            // Socket without a PCB (shouldn't happen)
            // protocol will show up as "raw" and there will be no stats
            // because PCB type is not known.
            stats_list.push_back(socket_pcb_stats{
                .pcb = pcb,
                .id = id,
                .channel_stats = get_socket_channel_stats(sock)});
            OP_LOG(OP_LOG_WARNING, "Found socket with no pcb");
        }
    });
    return stats_list;
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