#ifndef _OP_PACKET_STACK_DPDK_SYS_MAILBOX_HPP_
#define _OP_PACKET_STACK_DPDK_SYS_MAILBOX_HPP_

#include <atomic>
#include <memory>

#include "packetio/drivers/dpdk/dpdk.h"

/**
 * Note: this is the implementation of lwIP's sys_mbox_t object.
 */
struct sys_mbox
{
    struct rte_ring_deleter
    {
        void operator()(rte_ring* ring) { rte_ring_free(ring); }
    };

    static std::atomic_size_t m_idx;
    struct alignas(64)
    {
        std::unique_ptr<rte_ring, rte_ring_deleter> ring;
        int fd = -1;
        std::atomic_uint64_t read_idx = 0;
    } m_stack;
    struct alignas(64)
    {
        std::atomic_uint64_t write_idx = 0;
    } m_workers;

    uint64_t count() const;
    int notify();
    int ack();
    int ack_wait();

public:
    sys_mbox(int size);
    ~sys_mbox();

    sys_mbox(sys_mbox&& other) = delete;
    sys_mbox& operator=(sys_mbox&& other) = delete;

    int fd() const;
    void clear_notifications();

    void post(void* msg);
    bool trypost(void* msg);

    void* fetch(uint32_t timeout);
    void* tryfetch();
};

#endif /* _OP_PACKET_STACK_DPDK_SYS_MAILBOX_HPP_ */
