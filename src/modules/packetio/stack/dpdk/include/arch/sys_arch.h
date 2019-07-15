#ifndef _ICP_PACKETIO_STACK_DPDK_SYS_ARCH_H_
#define _ICP_PACKETIO_STACK_DPDK_SYS_ARCH_H_

#include <stddef.h>

/* We use eventfd's as our semaphores, hence semaphores are ints */
typedef int sys_sem_t;
#define sys_sem_valid(sem)				(*sem > 0)
#define sys_sem_set_invalid(sem)        (*sem = -1)
sys_sem_t* sys_arch_sem_thread_get();
#define LWIP_NETCONN_THREAD_SEM_GET()   sys_arch_sem_thread_get()

/*
 * We don't actually use this anywhere, but it needs to be defined for
 * function prototypes
 */
typedef unsigned char sys_mutex_t;

struct sys_mbox;
typedef struct sys_mbox * sys_mbox_t;
#define sys_mbox_valid(mbox)            (((mbox) != NULL) && (*(mbox) != NULL))

int sys_mbox_fd(const sys_mbox_t *);  /* retrieve the fd of the underlying mbox */
void sys_mbox_clear_notifications(sys_mbox_t *);

struct sys_thread;
typedef struct sys_thread * sys_thread_t;

void sys_check_timeouts();
u32_t sys_timeouts_sleeptime();

#endif /* _ICP_PACKETIO_STACK_DPDK_SYS_ARCH_H_ */
