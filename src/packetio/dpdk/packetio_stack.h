#ifndef _PACKETIO_STACK_H_
#define _PACKETIO_STACK_H_

#include <stdint.h>

struct stack_args {
    struct rte_mempool *pool;
    uint16_t port_id;
};

int packetio_stack_run(void *arg);

#endif /* _PACKETIO_STACK_H_ */
