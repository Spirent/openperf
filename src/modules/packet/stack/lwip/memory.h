#ifndef _OP_PACKET_STACK_LWIP_MEMORY_H_
#define _OP_PACKET_STACK_LWIP_MEMORY_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void packet_stack_memp_initialize();
void* packet_stack_memp_alloc(const char* desc, size_t size);
void packet_stack_memp_free(void* ptr);

struct memp_desc;
int64_t packet_stack_memp_pool_avail(const struct memp_desc*);
int64_t packet_stack_memp_pool_max(const struct memp_desc*);
int64_t packet_stack_memp_pool_used(const struct memp_desc*);

struct pbuf;
struct pbuf* packet_stack_pbuf_alloc();
void packet_stack_pbuf_free(struct pbuf*);

#ifdef __cplusplus
}
#endif

#endif /* _OP_PACKET_STACK_LWIP_MEMORY_H_ */
