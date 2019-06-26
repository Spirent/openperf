#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>

#include <poll.h>
#include <sys/eventfd.h>

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/stack/dpdk/sys_mailbox.h"

#ifndef EFD_SEMAPHORE
#define EFD_SEMAPHORE 1
int eventfd(unsigned int initval, int flags);
int eventfd_read(int fd, uint64_t* value);
int eventfd_write(int fd, uint64_t value);
#endif

std::atomic_size_t sys_mbox::m_idx = ATOMIC_VAR_INIT(0);

sys_mbox::sys_mbox(int size, int flags)
    : m_armed(false)
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

int sys_mbox::fd() const
{
    return (m_fd);
}

static bool is_readable(int fd, u32_t timeout)
{
    struct pollfd to_poll = { .fd = fd,
                              .events = POLLIN };
    int n = poll(&to_poll, 1, timeout);
    return (n == 1
            ? to_poll.revents & POLLIN
            : false);
}

static void do_read(int fd)
{
    uint64_t counter = 0;
    auto error = eventfd_read(fd, &counter);
    if (error) ICP_LOG(ICP_LOG_ERROR, "Could not read fd %d: %s\n",
                       fd, strerror(errno));
}

static void do_write(int fd)
{
    auto error = eventfd_write(fd, 1UL);
    if (error) ICP_LOG(ICP_LOG_ERROR, "Could not write fd %d: %s\n",
                       fd, strerror(errno));
}

void sys_mbox::clear_notifications()
{
    if (m_armed.test_and_set(std::memory_order_acquire)) {
        do_read(m_fd);
    }
    m_armed.clear(std::memory_order_release);
}

void sys_mbox::post(void *msg)
{
    bool empty = rte_ring_empty(m_ring.get());
    while (rte_ring_enqueue(m_ring.get(), msg) == -ENOBUFS) {
        rte_pause();
    }
    if (empty && !m_armed.test_and_set(std::memory_order_acquire)) {
        do_write(m_fd);
    }
}

bool sys_mbox::trypost(void *msg)
{
    bool empty = rte_ring_empty(m_ring.get());
    if (rte_ring_enqueue(m_ring.get(), msg) != 0) return (false);
    if (empty && !m_armed.test_and_set(std::memory_order_acquire)) {
        do_write(m_fd);
    }
    return (true);
}

void* sys_mbox::fetch(u32_t timeout)
{
    while (rte_ring_empty(m_ring.get())) {
        auto lwip_to = (timeout == 0 ? -1 : timeout);
        if (!is_readable(m_fd, lwip_to)) break;
        clear_notifications();
    }

    return (tryfetch());
}

void* sys_mbox::tryfetch()
{
    void *msg = nullptr;
    return (rte_ring_dequeue(m_ring.get(), &msg) == 0
            ? msg
            : nullptr);
}

extern "C" {

err_t sys_mbox_new(sys_mbox_t* mboxp, int size)
{
    assert(mboxp);
    sys_mbox *m = new sys_mbox(size);
    SYS_STATS_INC_USED(mbox);
    *mboxp = m;
    return (ERR_OK);
}

void sys_mbox_free(sys_mbox_t* mboxp)
{
    assert(*mboxp);
    delete *mboxp;
    *mboxp = nullptr;
}

int sys_mbox_fd(const sys_mbox_t* mboxp)
{
    assert(mboxp);
    const sys_mbox_t mbox = *mboxp;
    return (mbox->fd());
}

void sys_mbox_clear_notifications(sys_mbox_t* mboxp)
{
    assert(mboxp);
    sys_mbox_t mbox = *mboxp;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_clear_notification: mbox %p fd %d\n",
                            (void*)mbox, mbox->fd()));

    mbox->clear_notifications();
}

err_t sys_mbox_trypost(sys_mbox_t* mboxp, void* msg)
{
    assert(mboxp);
    assert(msg);

    sys_mbox_t mbox = *mboxp;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p fd %d msg %p\n",
                            (void *)mbox, mbox->fd(), msg));

    return (mbox->trypost(msg) ? ERR_OK : ERR_BUF);
}

void sys_mbox_post(sys_mbox_t* mboxp, void* msg)
{
    assert(mboxp);
    assert(msg);

    sys_mbox_t mbox = *mboxp;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox %p fd %d msg %p\n",
                            (void *)mbox, mbox->fd(), msg));

    mbox->post(msg);
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t* mboxp, void** msgp)
{
    assert(mboxp);
    assert(msgp);

    sys_mbox_t mbox = *mboxp;

    auto msg = mbox->tryfetch();
    if (!msg) return (SYS_MBOX_EMPTY);

    *msgp = msg;
    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p fd %d msg %p\n",
                            (void *)mbox, mbox->fd(), *msgp));
    return (0);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t* mboxp, void** msgp, u32_t timeout)
{
    assert(mboxp);
    assert(msgp);

    sys_mbox_t mbox = *mboxp;

    auto start = std::chrono::steady_clock::now();
    auto msg = mbox->fetch(timeout);
    if (!msg) return (SYS_ARCH_TIMEOUT);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    *msgp = msg;

    LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p fd %d msg %p duration %lu\n",
                            (void *)mbox, mbox->fd(), *msgp, duration));
    return (duration);
}

}
