#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <optional>

#include <poll.h>
#include <sys/eventfd.h>

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "packetio/drivers/dpdk/dpdk.h"

#ifndef EFD_SEMAPHORE
#define EFD_SEMAPHORE 1
int eventfd(unsigned int initval, int flags);
int eventfd_read(int fd, uint64_t* value);
int eventfd_write(int fd, uint64_t value);
#endif

#ifndef _PACKETIO_DRIVER_DPDK_H_
struct rte_ring;
void rte_ring_free(rte_ring *);
rte_ring* rte_ring_create(const char*, unsigned, int, unsigned);
int rte_ring_full(rte_ring *);
int rte_ring_empty(rte_ring *);
int rte_ring_enqueue(rte_ring *, void*);
int rte_ring_dequeue(rte_ring *, void**);
#endif

struct sys_mbox
{
    struct rte_ring_deleter {
        void operator()(rte_ring *ring) {
            rte_ring_free(ring);
        }
    };

    static std::atomic_size_t m_idx;
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    std::unique_ptr<rte_ring, rte_ring_deleter> m_ring;
    int m_fd;

public:
    sys_mbox(int size, int flags = 0);
    ~sys_mbox();

    void post(void *msg);
    bool trypost(void *msg);

    std::optional<void*> fetch(u32_t timeout);
    std::optional<void*> tryfetch();
};

std::atomic_size_t sys_mbox::m_idx = ATOMIC_VAR_INIT(0);

sys_mbox::sys_mbox(int size, int flags)
{
    if (!size) size = 128;

    if (size == 0 || (size & (size - 1)) != 0) {
        throw std::runtime_error("size must be a non-zero power of two");
    }

    if ((m_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not open eventfd");
    }

    auto name = "sys_mbox_" + std::to_string(m_idx++);
    if (m_ring.reset(rte_ring_create(name.c_str(), size, -1, flags)); !m_ring) {
        throw std::runtime_error("Could not initialize ring");
    }
}

sys_mbox::~sys_mbox()
{
    close(m_fd);
}

void sys_mbox::post(void *msg)
{
    while (rte_ring_enqueue(m_ring.get(), msg) == -ENOBUFS) {
        eventfd_write(m_fd, eventfd_max);
    }
    eventfd_write(m_fd, static_cast<uint64_t>(1));
}

bool sys_mbox::trypost(void *msg)
{
    if (rte_ring_enqueue(m_ring.get(), msg) != 0) return (false);
    eventfd_write(m_fd, static_cast<uint64_t>(1));
    return (true);
}

std::optional<void*> sys_mbox::fetch(u32_t timeout)
{
    while (rte_ring_empty(m_ring.get())) {
        struct pollfd pfd = {
            .fd = m_fd,
            .events = POLLIN
        };
        int n = poll(&pfd, 1, timeout == 0 ? -1 : timeout);
        if (n == 0) break;  /* timeout */
        if (n == 1) {
            uint64_t counter = 0;
            eventfd_read(m_fd, &counter);
        }
    }

    return (tryfetch());
}

std::optional<void*> sys_mbox::tryfetch()
{
    void *msg = nullptr;
    return (rte_ring_dequeue(m_ring.get(), &msg) == 0
            ? std::make_optional(msg)
            : std::nullopt);
}

extern "C" {

err_t sys_mbox_new(sys_mbox** mbox, int size)
{
    assert(mbox);
    sys_mbox *m = new sys_mbox(size);
    SYS_STATS_INC_USED(mbox);
    *mbox = m;
    return (ERR_OK);
}

void sys_mbox_free(sys_mbox** mbox)
{
    assert(*mbox);
    delete *mbox;
    *mbox = nullptr;
}

err_t sys_mbox_trypost(sys_mbox** mboxp, void* msg)
{
    assert(mboxp);
    assert(msg);

    sys_mbox* mbox = *mboxp;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p msg %p\n",
                            (void *)mbox, (void *)msg));

    return (mbox->trypost(msg) ? ERR_OK : ERR_BUF);
}

void sys_mbox_post(sys_mbox** mboxp, void* msg)
{
    assert(mboxp);
    assert(msg);

    sys_mbox* mbox = *mboxp;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox %p msg %p\n",
                            (void *)mbox, (void *)msg));

    mbox->post(msg);
}

u32_t sys_arch_mbox_tryfetch(sys_mbox** mboxp, void** msgp)
{
    assert(mboxp);
    assert(msgp);

    sys_mbox* mbox = *mboxp;

    auto msg = mbox->tryfetch();
    if (!msg) return (SYS_MBOX_EMPTY);

    *msgp = *msg;
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p msg %p\n",
                            (void *)mbox, *msgp));
    return (0);
}

u32_t sys_arch_mbox_fetch(sys_mbox** mboxp, void** msgp, u32_t timeout)
{
    assert(mboxp);
    assert(msgp);

    sys_mbox* mbox = *mboxp;

    auto start = std::chrono::steady_clock::now();
    auto msg = mbox->fetch(timeout);
    if (!msg) return (SYS_ARCH_TIMEOUT);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    *msgp = *msg;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p msg %p duration %lu\n",
                            (void *)mbox, *msgp, duration));
    return (duration);
}

}
