#ifndef _OP_PACKETIO_MEMORY_DPDK_MEMORY_H_
#define _OP_PACKETIO_MEMORY_DPDK_MEMORY_H_

#include <stdint.h>
#include <stddef.h>

#include "lwip/memp.h"

#ifdef __cplusplus
extern "C" {
#endif

void packetio_memory_initialize();
void* packetio_memory_alloc(const char* desc, size_t size);
void packetio_memory_free(void* ptr);

struct pbuf;
struct pbuf* packetio_memory_pbuf_default_alloc();
struct pbuf* packetio_memory_pbuf_ref_rom_alloc();
void packetio_memory_pbuf_free(struct pbuf*);

struct memp_desc;
int64_t packetio_memory_memp_pool_avail(const struct memp_desc*);
int64_t packetio_memory_memp_pool_max(const struct memp_desc*);
int64_t packetio_memory_memp_pool_used(const struct memp_desc*);

#ifdef __cplusplus
}
#endif

#endif /* _OP_PACKETIO_MEMORY_DPDK_MEMORY_H_ */
