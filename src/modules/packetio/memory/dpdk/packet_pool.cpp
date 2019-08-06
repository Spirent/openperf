#include <algorithm>

#include "core/icp_log.h"
#include "packetio/memory/dpdk/packet_pool.h"

namespace icp::packetio::dpdk {

/*
 * Convert nb_mbufs to a Mersenne number, as those are the
 * most efficient size for mempools.  If our input is already a
 * power of 2, return input - 1 instead of doubling the size.
 */
__attribute__((const))
static uint32_t pool_size_adjust(uint32_t nb_mbufs)
{
    return (rte_is_power_of_2(nb_mbufs)
            ? nb_mbufs - 1
            : rte_align32pow2(nb_mbufs) - 1);
}

rte_mempool* create_mempool(std::string_view id, int numa_mode,
                            uint16_t max_packet_length, uint16_t num_packets)
{
    static constexpr auto max_length = RTE_MEMPOOL_NAMESIZE - 1;
    auto name = "pool-" + std::string(id);
    if (name.length() >= max_length) {
        ICP_LOG(ICP_LOG_DEBUG, "Truncating mempool name = %s\n", name.c_str());
        name.resize(max_length);
        name.resize(name.find_last_of('-'));  /* Pick a nice trim spot for uuids */
    }

    return (rte_pktmbuf_pool_create(name.c_str(), pool_size_adjust(num_packets),
                                    0, 0, max_packet_length, numa_mode));
}

packet_pool::packet_pool(std::string_view id, int numa_node,
                         uint16_t max_packet_length, uint16_t num_packets)
    : m_pool(create_mempool(id, numa_node, max_packet_length, num_packets),
             [](auto p) {
                 ICP_LOG(ICP_LOG_DEBUG, "Deleting packet pool %s\n", p->name);
                 rte_mempool_free(p);
             })
{
    if (!m_pool) {
        throw std::runtime_error("Could not create DPDK packet pool: "
                                 + std::string(rte_strerror(rte_errno)));
    }

    ICP_LOG(ICP_LOG_DEBUG, "%s: %u, %u byte mbufs on NUMA socket %d\n",
            m_pool->name,
            m_pool->size,
            rte_pktmbuf_data_room_size(m_pool.get()),
            m_pool->socket_id);
}

packets::packet_buffer* packet_pool::get()
{
    return (reinterpret_cast<packets::packet_buffer*>(rte_pktmbuf_alloc(m_pool.get())));
}

uint16_t packet_pool::get(packets::packet_buffer* packets[], uint16_t count)
{
    auto error = rte_pktmbuf_alloc_bulk(m_pool.get(),
                                        reinterpret_cast<rte_mbuf**>(packets), count);
    return (error ? 0 : count);
}

void packet_pool::put(packets::packet_buffer* packet)
{
    rte_pktmbuf_free(reinterpret_cast<rte_mbuf*>(packet));
}

void packet_pool::put(packets::packet_buffer* const packets[], uint16_t count)
{
    std::for_each(packets, packets + count,
                  [](auto packet){
                      rte_pktmbuf_free(reinterpret_cast<rte_mbuf*>(packet));
                  });
}

}
