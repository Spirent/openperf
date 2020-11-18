#include "lwip/opt.h"

#if LWIP_PACKET /* don't build if not configured for use in lwipopts.h */

#include <cassert>
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

/* Linux headers */
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/if_arp.h>
#include <net/ethernet.h>

/* LwIP headers */
#include "lwip/ip.h"
#include "lwip/prot/ethernet.h"
#include "netif/ethernet.h"
#include "stack/lwip/packet.h"

struct packet_pcb
{
    IP_PCB; /* needed for {get,set}sockopt compatibility */

    packet_recv_fn recv = nullptr;
    void* packet_arg = nullptr;
    packet_type_t type = PACKET_TYPE_NONE;
    u16_t proto = 0;
    struct {
        unsigned nb_packets = 0;
        unsigned nb_drops = 0;
    } stats;
};

/* Container for all system pcbs */
static std::vector<std::unique_ptr<packet_pcb>> packet_pcbs;

static bool packet_match(const struct packet_pcb* pcb,
                        const struct netif* inp,
                        const struct eth_hdr* ethhdr)
{
    if (!pcb->recv) { return (false); }

    if (pcb->netif_idx != netif_get_index(inp)) {
        return (false);
    }

    if (pcb->proto == PP_HTONS(ETH_P_ALL)
        || pcb->proto == ethhdr->type) { return (true); }

    return (false);
}

extern "C" {

void packet_input(struct pbuf* p, struct netif* inp)
{
    if (p->len <= SIZEOF_ETH_HDR) {
        /* We can't do anything with this */
        return;
    }

    auto* ethhdr = reinterpret_cast<eth_hdr*>(p->payload);

    std::for_each(std::begin(packet_pcbs), std::end(packet_pcbs), [&](const auto& pcb){
        if (packet_match(pcb.get(), inp, ethhdr)) {
            /* Make a clone for the packet socket to consume */
            if (auto* clone = pbuf_clone(PBUF_RAW, PBUF_POOL, p)) {

                LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_TRACE,
                            ("packet_input: forwarding datagram from %c%c%u\n",
                             inp->name[0], inp->name[1], inp->num));

                /* Generate sockaddr_ll data */
                struct sockaddr_ll sa = {
                    .sll_family = AF_PACKET,
                    .sll_protocol = lwip_htons(ethhdr->type),
                    .sll_ifindex = netif_get_index(inp),
                    .sll_hatype = ARPHRD_ETHER,
                    .sll_pkttype = PACKET_HOST,
                    .sll_halen = ETH_HWADDR_LEN};

                if (ethhdr->dest.addr[0] & 1) {
                    sa.sll_pkttype = (eth_addr_cmp(&ethhdr->dest, &ethbroadcast)
                                      ? PACKET_BROADCAST
                                      : PACKET_MULTICAST);
                }

                if (pcb->type == PACKET_TYPE_DGRAM) {
                    /* We need to trim the header from the clone */
                    u16_t to_drop = SIZEOF_ETH_HDR;
#if ETHARP_SUPPORT_VLAN
                    if (proto == PP_HTONS(ETHTYPE_VLAN)) {
                        to_drop = SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR;
                    }
#endif
                    pbuf_remove_header(p, to_drop);
                }

                pcb->stats.nb_packets++;
                if (pcb->recv(pcb->packet_arg, clone, &sa) != ERR_OK) {
                    pcb->stats.nb_drops++;
                }
            }
        }
    });
}

struct packet_pcb* packet_new(packet_type_t type, u16_t proto)
{
    if (type == PACKET_TYPE_NONE) { return (nullptr); }

    LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_TRACE, ("packet_new\n"));

    packet_pcbs.push_back(std::make_unique<packet_pcb>());
    auto& pcb = packet_pcbs.back();
    pcb->type = type;
    pcb->proto = proto;
    return (pcb.get());
}

void packet_remove(struct packet_pcb* pcb)
{
    if (!pcb) { return; }

    LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_TRACE, ("packet_remove: %p\n",
                                                reinterpret_cast<void*>(pcb)));

    packet_pcbs.erase(std::remove_if(std::begin(packet_pcbs),
                                     std::end(packet_pcbs),
                                     [&](const auto& ptr) {
                                         return (ptr.get() == pcb);
                                     }),
                      std::end(packet_pcbs));
}

err_t packet_bind(struct packet_pcb* pcb, u16_t proto, int ifindex)
{
    auto* ifp = netif_get_by_index(static_cast<uint8_t>(ifindex));
    if (!ifp) { return (ERR_IF); }

    pcb->netif_idx = ifindex;
    pcb->proto = proto;

    return (ERR_OK);
}

err_t packet_send(struct packet_pcb* pcb, struct pbuf* p)
{
    LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_TRACE, ("packet_send\n"));

    if (pcb->type == PACKET_TYPE_DGRAM
        || pcb->netif_idx == NETIF_NO_INDEX) {
        LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_LEVEL_WARNING,
                    ("packet_send: no destination!\n"));
        return (ERR_RTE);
    }

    auto* ifp = netif_get_by_index(pcb->netif_idx);
    assert(ifp);
    return (ifp->linkoutput(ifp, p));
}

err_t packet_sendto(struct packet_pcb* pcb, struct pbuf* p, const struct sockaddr_ll* sa)
{
    LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_TRACE, ("packet_sendto\n"));

    auto* ifp = netif_get_by_index(static_cast<uint8_t>(sa->sll_ifindex));
    if (!ifp) {
        LWIP_DEBUGF(PACKET_DEBUG | LWIP_DBG_LEVEL_WARNING,
                    ("packet_sendto: No interface for index:%" U16_F "\n",
                     sa->sll_ifindex));
        return (ERR_IF);
    }

    return (pcb->type == PACKET_TYPE_DGRAM
            ? ethernet_output(ifp, p,
                              reinterpret_cast<const struct eth_addr*>(ifp->hwaddr),
                              reinterpret_cast<const struct eth_addr*>(sa->sll_addr),
                              sa->sll_protocol)
            : ifp->linkoutput(ifp, p));
}

void packet_recv(struct packet_pcb* pcb, packet_recv_fn recv, void* packet_arg)
{
    pcb->recv = recv;
    pcb->packet_arg = packet_arg;
}

err_t packet_getsockname(const struct packet_pcb* pcb, struct sockaddr_ll* sll)
{
    LWIP_ASSERT("Packet PCB must be bound", pcb->netif_idx == NETIF_NO_INDEX);

    auto* ifp = netif_get_by_index(pcb->netif_idx);
    assert(ifp);

    sll->sll_family = AF_PACKET;
    sll->sll_protocol = pcb->proto;
    sll->sll_ifindex = pcb->netif_idx;
    sll->sll_hatype = ARPHRD_ETHER;
    sll->sll_halen = ifp->hwaddr_len;
    std::copy_n(ifp->hwaddr,
                std::min(static_cast<size_t>(ifp->hwaddr_len), sizeof(sll->sll_addr)),
                sll->sll_addr);

    return (ERR_OK);
}

packet_type_t packet_getsocktype(const struct packet_pcb* pcb)
{
    return (pcb->type);
}

unsigned packet_stat_total(const struct packet_pcb* pcb)
{
    return (pcb->stats.nb_packets);
}

unsigned packet_stat_drops(const struct packet_pcb* pcb)
{
    return (pcb->stats.nb_drops);
}

}

#endif /* LWIP_PACKET */
