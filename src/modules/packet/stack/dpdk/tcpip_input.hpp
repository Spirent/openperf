#ifndef _OP_PACKET_STACK_DPDK_TCPIP_INPUT_HPP_
#define _OP_PACKET_STACK_DPDK_TCPIP_INPUT_HPP_

#include <atomic>
#include <memory>
#include <variant>

#include "lwip/err.h"

#include "utils/singleton.hpp"

struct netif;
struct pbuf;
struct rte_mbuf;
struct rte_ring;

extern "C" void rte_ring_free(struct rte_ring*);

namespace openperf::packet::stack::dpdk {

namespace impl {

struct tcpip_input_direct
{
    static err_t inject(struct netif* ifp, rte_mbuf* packet);
};

class tcpip_input_queue
{
    struct rte_ring_deleter
    {
        void operator()(rte_ring* ring) { rte_ring_free(ring); }
    };

    std::unique_ptr<rte_ring, rte_ring_deleter> m_queue;
    std::atomic_flag m_notify = false;
    static constexpr int rx_queue_size = 4096;
    static constexpr auto ring_name = "tcpip_input_ring";

public:
    tcpip_input_queue();
    ~tcpip_input_queue() = default;

    err_t inject(struct netif* ifp, rte_mbuf* packet);

    uint16_t dequeue(pbuf* packets[], uint16_t max_packets);

    void ack();
};

} // namespace impl

class tcpip_input : public utils::singleton<tcpip_input>
{
    using input_type =
        std::variant<impl::tcpip_input_direct, impl::tcpip_input_queue>;
    input_type m_input;

public:
    void init();
    void fini();

    err_t inject(netif* netif, rte_mbuf* packet);
};

} // namespace openperf::packet::stack::dpdk
#endif /* _OP_PACKET_STACK_DPDK_TCPIP_INPUT_HPP_ */
