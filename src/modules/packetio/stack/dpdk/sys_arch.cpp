#include <cerrno>
#include <chrono>

#include <poll.h>

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include <sys/eventfd.h>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"

#ifndef EFD_SEMAPHORE
#define EFD_SEMAPHORE 1
int eventfd(unsigned int initval, int flags);
int eventfd_read(int fd, uint64_t* value);
int eventfd_write(int fd, uint64_t value);
#endif

namespace lwip {
/* XXX: need to add a shutdown hook to properly close these descriptors */
static thread_local sys_sem_t thread_fd = -1;
icp::list<int> thread_semaphores;
}

extern "C" {

void sys_init() {}

/***
 * Semaphores
 ***/

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    *sem = eventfd(count, EFD_SEMAPHORE);
    return (*sem == -1 ? ERR_VAL : ERR_OK);
}

void sys_sem_free(sys_sem_t *sem)
{
    close(*sem);
}

void sys_sem_signal(sys_sem_t *sem)
{
    uint64_t incr = 1;
    eventfd_write(*sem, incr);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    auto start = std::chrono::steady_clock::now();

    struct pollfd pfd = {
        .fd = *sem,
        .events = POLLIN
    };

    int n = poll(&pfd, 1, timeout == 0 ? -1 : timeout);
    if (n <= 0) return (SYS_ARCH_TIMEOUT);

    uint64_t counter = 0;
    eventfd_read(*sem, &counter);

    return (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());
}

sys_sem_t* sys_arch_sem_thread_get()
{
    if (lwip::thread_fd == -1) {
        auto err = sys_sem_new(&lwip::thread_fd, 0);
        if (err != ERR_OK) return (nullptr);

        lwip::thread_semaphores.insert(&lwip::thread_fd);
    }

    return (&lwip::thread_fd);
}

/***
 * System time
 ***/

u32_t sys_now(void)
{
    return (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
}

/***
 * Threads
 ***/

struct thread_wrapper_data
{
    const char* name;
    lwip_thread_fn function;
    void *arg;
};

static int
thread_wrapper(void *arg)
{
    struct thread_wrapper_data *thread_data = (struct thread_wrapper_data *)arg;

    icp_thread_setname(thread_data->name);
    thread_data->function(thread_data->arg);

    /* we should never get here */
    free(arg);
    return (0);
}

static int get_waiting_lcore()
{
    int i = 0;
    RTE_LCORE_FOREACH_SLAVE(i) {
        switch (rte_eal_get_lcore_state(i)) {
        case FINISHED:
            rte_eal_wait_lcore(i);
            break;
        case WAIT:
            return (i);
        default:
            continue;
        }
    }

    return (-1);
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function,
                            void *arg, int stacksize, int prio)
{
    LWIP_UNUSED_ARG(stacksize);
    LWIP_UNUSED_ARG(prio);

    int lcore = -1;
    if ((lcore = get_waiting_lcore()) < 0) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_thread_new: lcore = %d", lcore));
        abort();
    }

    auto thread_data = reinterpret_cast<thread_wrapper_data*>(malloc(sizeof(struct thread_wrapper_data)));
    thread_data->name = name;
    thread_data->arg = arg;
    thread_data->function = function;
    int code = rte_eal_remote_launch(thread_wrapper, thread_data, lcore);

    if (code) {
        LWIP_DEBUGF(SYS_DEBUG, ("sys_thread_new: rte_eal_remote_launch %s: %d", name, code));
        abort();
    }

    return (nullptr);
}

}
