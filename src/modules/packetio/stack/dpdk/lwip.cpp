#include "lwipopts.h"
#include "lwip/tcpip.h"

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/topology_utils.h"
#include "packetio/stack/dpdk/netif_wrapper.h"
#include "packetio/stack/dpdk/lwip.h"

namespace icp {
namespace packetio {
namespace dpdk {

static void tcpip_init_done(void *arg __attribute__((unused)))
{
    ICP_LOG(ICP_LOG_DEBUG, "TCP/IP thread running on logical core %u (NUMA node %u)\n",
            rte_lcore_id(), rte_socket_id());
}

lwip::lwip(driver::generic_driver& driver)
    : m_driver(driver)
    , m_idx(0)
{
    tcpip_init(tcpip_init_done, nullptr);
}

std::vector<int> lwip::interface_ids() const
{
    std::vector<int> ids;
    for (auto& item : m_interfaces) {
        ids.push_back(item.first);
    }

    return (ids);
}

static netif_wrapper make_netif_wrapper(const std::unique_ptr<net_interface>& ifp)
{
    return (netif_wrapper(ifp->id(), ifp->data(), ifp->config()));
}

std::optional<interface::generic_interface> lwip::interface(int id) const
{
    return (m_interfaces.find(id) != m_interfaces.end()
            ? std::make_optional(interface::generic_interface(
                                     make_netif_wrapper(m_interfaces.at(id))))
            : std::nullopt);
}

tl::expected<int, std::string> lwip::create_interface(const interface::config_data& config)
{
    try {
        auto ifp = std::make_unique<net_interface>(m_idx, config, m_driver.tx_burst_function(config.port_id));
        m_driver.add_interface(ifp->port_id(), ifp->data());
        auto item = m_interfaces.emplace(std::make_pair(m_idx++, std::move(ifp)));
        return (item.first->first);
    } catch (const std::runtime_error &e) {
        ICP_LOG(ICP_LOG_ERROR, "Interface creation failed: %s\n", e.what());
        return (tl::make_unexpected(e.what()));
    }
}

void lwip::delete_interface(int id)
{
    if (m_interfaces.find(id) != m_interfaces.end()) {
        auto& ifp = m_interfaces.at(id);
        m_driver.del_interface(ifp->port_id(), std::make_any<netif*>(ifp->data()));
        m_interfaces.erase(id);
    }
}

extern "C" err_t tcpip_shutdown();
void lwip::shutdown() const
{
    if (tcpip_shutdown() == ERR_OK) {
        int id = icp::packetio::dpdk::topology::get_stack_lcore_id();
        rte_eal_wait_lcore(id);
    }
}

/**
 * The rest of this file is pretty ugly.
 * The LwIP code guards all stats via ifdef blocks.  We need to do the same
 * here so that our retrieval code matches what actually exists.
 * Additionally, the ifdef guards around the functions prevent the compiler
 * from whining about unused functions.
 */

#if LWIP_STATS
#if SYS_STATS
static stack::element_stats_data make_element_stats_data(const stats_syselem* syselem)
{
    return { .used   = syselem->used,
             .max    = syselem->max,
             .errors = syselem->err };
}
#endif

#if MEMP_STATS
static stack::memory_stats_data make_memory_stats_data(const memp_desc* mem)
{
    return { .name      = mem->desc,
             .available = static_cast<int64_t>(mem->stats->avail),
             .used      = static_cast<int64_t>(mem->stats->used),
             .max       = static_cast<int64_t>(mem->stats->max),
             .errors    = mem->stats->err,
             .illegal   = mem->stats->illegal };
}
#endif

#if IGMP_STATS || MLD6_STATS
static stack::protocol_stats_data make_protocol_stats_data(const stats_igmp *igmp)
{
        return { .tx_packets         = igmp->xmit,
                 .rx_packets         = igmp->recv,
                 .dropped_packets    = igmp->drop,
                 .checksum_errors    = igmp->chkerr,
                 .length_errors      = igmp->lenerr,
                 .memory_errors      = igmp->memerr,
                 .protocol_errors    = igmp->proterr };
}
#endif

static stack::protocol_stats_data make_protocol_stats_data(const stats_proto* proto)
{
    return { .tx_packets        = proto->xmit,
             .rx_packets        = proto->recv,
             .forwarded_packets = proto->fw,
             .dropped_packets   = proto->drop,
             .checksum_errors   = proto->chkerr,
             .length_errors     = proto->lenerr,
             .memory_errors     = proto->memerr,
             .routing_errors    = proto->rterr,
             .protocol_errors   = proto->proterr,
             .option_errors     = proto->opterr,
             .misc_errors       = proto->err,
             .cache_hits        = proto->cachehit };
}
#endif /* LWIP_STATS */

std::unordered_map<std::string, stack::stats_data> lwip::stats() const
{
    std::unordered_map<std::string, stack::stats_data> stats;

#if LWIP_STATS
#if SYS_STATS
    stats.emplace("sems",      make_element_stats_data(&lwip_stats.sys.sem));
    stats.emplace("mutexes",   make_element_stats_data(&lwip_stats.sys.mutex));
    stats.emplace("mboxes",    make_element_stats_data(&lwip_stats.sys.mbox));
#endif

#if MEMP_STATS
    /**
     * The mem_std.h file contains a LWIP_MEMPOOL() entry for every mempool
     * used by the stack.  Additionally, those mempool/macro blocks are
     * all guarded by ifdefs, so they always match the actual LwIP
     * compilation options.  Hence, we use the following macro + #include
     * to generate our memory statistics.
     */
#define LWIP_MEMPOOL(pool_name, ...)                                    \
    stats.emplace(memp_pools[MEMP_##pool_name]->desc,                   \
                  make_memory_stats_data(memp_pools[MEMP_##pool_name]));
#include "lwip/priv/memp_std.h"
#endif

#if ETHARP_STATS
    stats.emplace("arp",       make_protocol_stats_data(&lwip_stats.etharp));
#endif

#if IP_STATS
    stats.emplace("ipv4",      make_protocol_stats_data(&lwip_stats.ip));
#endif

#if IPFRAG_STATS
    stats.emplace("ipv4_frag", make_protocol_stats_data(&lwip_stats.ip_frag));
#endif

#if ICMP_STATS
    stats.emplace("icmpv4",    make_protocol_stats_data(&lwip_stats.icmp));
#endif

#if IGMP_STATS
    stats.emplace("igmp",      make_protocol_stats_data(&lwip_stats.igmp));
#endif

#if ND6_STATS
    stats.emplace("nd",        make_protocol_stats_data(&lwip_stats.nd6));
#endif

#if IP6_STATS
    stats.emplace("ipv6",      make_protocol_stats_data(&lwip_stats.ip6));
#endif

#if IP6_FRAG_STATS
    stats.emplace("ipv6_frag", make_protocol_stats_data(&lwip_stats.ip6_frag));
#endif

#if ICMP6_STATS
    stats.emplace("icmpv6",    make_protocol_stats_data(&lwip_stats.icmp6));
#endif

#if MLD6_STATS
    stats.emplace("mld",       make_protocol_stats_data(&lwip_stats.mld6));
#endif

#if UDP_STATS
    stats.emplace("udp",       make_protocol_stats_data(&lwip_stats.udp));
#endif

#if TCP_STATS
    stats.emplace("tcp",       make_protocol_stats_data(&lwip_stats.tcp));
#endif
#endif /* LWIP_STATS */

    return (stats);
}

}
}
}
