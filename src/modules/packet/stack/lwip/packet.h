#ifndef _OP_PACKET_STACK_LWIP_PACKET_H_
#define _OP_PACKET_STACK_LWIP_PACKET_H_

#include "lwip/opt.h"

#if LWIP_PACKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip.h"

#ifdef __cplusplus
extern "C" {
#endif

struct packet_pcb;
struct sockaddr_ll;

typedef err_t (*packet_recv_fn)(void *arg,
                                struct pbuf* p,
                                const struct sockaddr_ll* sa);

typedef enum packet_type {
    PACKET_TYPE_NONE = 0,
    PACKET_TYPE_DGRAM,
    PACKET_TYPE_RAW
} packet_type_t;

struct packet_pcb* packet_new(packet_type_t type, u16_t proto);
void               packet_remove(struct packet_pcb* pcb);
err_t              packet_bind(struct packet_pcb* pcb, u16_t proto, int ifindex);
err_t              packet_send(struct packet_pcb* pcb, struct pbuf* p);
err_t              packet_sendto(struct packet_pcb* pcb, struct pbuf* p, const struct sockaddr_ll* sll);
void               packet_recv(struct packet_pcb* pcb, packet_recv_fn recv, void* packet_arg);
err_t              packet_getsockname(const struct packet_pcb* pcb, struct sockaddr_ll* sll);
packet_type_t      packet_getsocktype(const struct packet_pcb* pcb);
void               packet_set_promiscuous(struct packet_pcb* pcb, uint8_t ifindex, int enable);
void               packet_set_multicast(struct packet_pcb* pcb, uint8_t ifindex, int enable);
err_t              packet_add_membership(struct packet_pcb* pcb, uint8_t ifindex, const uint8_t addr[8]);
void               packet_drop_membership(struct packet_pcb* pcb, uint8_t ifindex, const uint8_t addr[8]);
unsigned           packet_stat_total(const struct packet_pcb* pcb);
unsigned           packet_stat_drops(const struct packet_pcb* pcb);
uint32_t           packet_get_ifindex(const struct packet_pcb *pcb);
packet_type_t      packet_get_packet_type(const struct packet_pcb *pcb);
uint16_t            packet_get_proto(const struct packet_pcb *pcb);
/*
 * Matching packets are copied to sockets, so packets are never
 * consumed by this function.
 */
void packet_input(struct pbuf* p, struct netif* inp);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_PACKET */

#endif /* _OP_PACKET_STACK_LWIP_PACKET_H_ */
