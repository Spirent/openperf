#ifndef _ICP_PACKETIO_MEMORY_DPDK_MEMP_H_
#define _ICP_PACKETIO_MEMORY_DPDK_MEMP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char memp_default_mempool_fmt[];
extern const char memp_ref_rom_mempool_fmt[];

struct memp_desc;
int64_t packetio_memory_memp_pool_avail(const struct memp_desc*);
int64_t packetio_memory_memp_pool_max(const struct memp_desc*);
int64_t packetio_memory_memp_pool_used(const struct memp_desc*);

#ifdef __cplusplus
}
#endif

#endif /* _ICP_PACKETIO_MEMORY_DPDK_MEMP_H_ */
