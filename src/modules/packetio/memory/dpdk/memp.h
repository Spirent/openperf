#ifndef _ICP_PACKETIO_MEMORY_DPDK_MEMP_H_
#define _ICP_PACKETIO_MEMORY_DPDK_MEMP_H_

#include "lwip/pbuf.h"
#include "packetio/drivers/dpdk/dpdk.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char memp_default_mempool[];
extern const char memp_ref_rom_mempool[];

/*
 * We stuff the pbuf header in the DPDK mbuf private data area, so
 * that the stack can use DPDK mbufs directly without any additional
 * overhead.  This also means we can translate pbuf <--> mbuf by
 * just adding or subtracting an offset.
 * Note: this only works because the headers are nicely aligned.
 */
inline struct rte_mbuf * packetio_memp_pbuf_to_mbuf(const struct pbuf *pbuf)
{
    return (pbuf != NULL
            ? (struct rte_mbuf *)((uintptr_t)(pbuf) - sizeof(struct rte_mbuf))
            : NULL);
}

inline struct pbuf * packetio_memp_mbuf_to_pbuf(const struct rte_mbuf *mbuf)
{
    return (mbuf != NULL
            ? (struct pbuf *)((uintptr_t)(mbuf) + sizeof(struct rte_mbuf))
            : NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* _ICP_PACKETIO_MEMORY_DPDK_MEMP_H_ */
