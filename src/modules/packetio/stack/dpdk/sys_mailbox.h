#ifndef _ICP_PACKETIO_DPDK_SYS_MAILBOX_H_
#define _ICP_PACKETIO_DPDK_SYS_MAILBOX_H_

#include <atomic>
#include <memory>

#include "packetio/drivers/dpdk/dpdk.h"

/**
 * Note: this is the implementation of lwIP's sys_mbox_t object.
 */
struct sys_mbox
{
    struct rte_ring_deleter {
        void operator()(rte_ring *ring) {
            rte_ring_free(ring);
        }
    };

    static std::atomic_size_t m_idx;
    std::unique_ptr<rte_ring, rte_ring_deleter> m_ring;
    std::atomic_flag m_armed;
    int m_fd;

public:
    sys_mbox(int size = 32, int flags = RING_F_SC_DEQ);
    ~sys_mbox();

    sys_mbox(sys_mbox&& other) = delete;
    sys_mbox& operator=(sys_mbox&& other) = delete;

    int fd() const;
    void clear_notifications();

    void post(void *msg);
    bool trypost(void *msg);

    void* fetch(uint32_t timeout);
    void* tryfetch();
};

#endif /* _ICP_PACKETIO_DPDK_SYS_MAILBOX_H_ */
