#ifndef _ICP_PACKETIO_MEMORY_DPDK_PBUF_UTILS_H_
#define _ICP_PACKETIO_MEMORY_DPDK_PBUF_UTILS_H_

#include "lwip/pbuf.h"
#include "packetio/drivers/dpdk/dpdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Translate a {pbuf,mbuf} pointer into a {mbuf, pbuf} pointer */
struct pbuf* packetio_memory_mbuf_to_pbuf(const struct rte_mbuf*);
struct rte_mbuf* packetio_memory_pbuf_to_mbuf(const struct pbuf*);

/* Synchronize packet meta data between mbuf and pbuf */
struct pbuf* packetio_memory_pbuf_synchronize(struct rte_mbuf*);
struct rte_mbuf* packetio_memory_mbuf_synchronize(struct pbuf*);

#ifdef __cplusplus
}
#endif

#endif /* _ICP_PACKETIO_MEMORY_DPDK_PBUF_UTILS_H_ */
