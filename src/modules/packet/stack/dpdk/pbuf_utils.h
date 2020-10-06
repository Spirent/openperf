#ifndef _OP_PACKET_STACK_DPDK_PBUF_UTILS_H_
#define _OP_PACKET_STACK_DPDK_PBUF_UTILS_H_

#include "lwip/pbuf.h"
#include "packetio/drivers/dpdk/dpdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Translate a {pbuf,mbuf} pointer into a {mbuf, pbuf} pointer */
struct pbuf* packet_stack_mbuf_to_pbuf(const struct rte_mbuf*);
struct rte_mbuf* packet_stack_pbuf_to_mbuf(const struct pbuf*);

/* Synchronize packet meta data between mbuf and pbuf */
struct pbuf* packet_stack_pbuf_synchronize(struct rte_mbuf*);
struct rte_mbuf* packet_stack_mbuf_synchronize(struct pbuf*);

/* Report the amount of space after/before the payload pointer, respectively */
uint16_t packet_stack_pbuf_data_available(const struct pbuf*);
uint16_t packet_stack_pbuf_header_available(const struct pbuf*);

#ifdef __cplusplus
}
#endif

#endif /* _OP_PACKET_STACK_DPDK_PBUF_UTILS_H_ */
