#include "lwipopts.h"
#include "lwip/tcpip.h"

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/stack/dpdk/lwip.h"

namespace icp {
namespace packetio {
namespace dpdk {

static void tcpip_init_done(void *arg __attribute__((unused)))
{
    ICP_LOG(ICP_LOG_DEBUG, "TCP/IP thread running on logical core %u\n",
            rte_lcore_id());
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

std::optional<interface::generic_interface> lwip::interface(int id) const
{
    return (m_interfaces.find(id) != m_interfaces.end()
            ? std::make_optional(interface::generic_interface(*m_interfaces.at(id).get()))
            : std::nullopt);
}

tl::expected<int, std::string> lwip::create_interface(const interface::config_data& config)
{
    try {
        auto item = m_interfaces.emplace(
            std::make_pair(m_idx, std::make_unique<net_interface>(m_idx, config)));
        m_idx++;
        return (item.first->first);
    } catch (const std::runtime_error &e) {
        ICP_LOG(ICP_LOG_ERROR, "Interface creation failed: %s\n", e.what());
        return (tl::make_unexpected(e.what()));
    }
}

void lwip::delete_interface(int id)
{
    m_interfaces.erase(id);
}

/**
 * LwIP stats are all guarded by ifdef blocks, so we need to do the
 * same to make sure our stats retrieval functions match our compilation
 * options.
 * Ifdef'ing the functions is to prevent the compiler warning us about
 * unused functions.
 */


#if SYS_STATS
static stack::element_stats_data make_element_stats_data(const stats_syselem* syselem)
{
    return { .used   = syselem->used,
             .max    = syselem->max,
             .errors = syselem->max };
}
#endif

static stack::memory_stats_data make_memory_stats_data(const memp_desc* mem)
{
    return { .name      = mem->desc,
             .available = static_cast<int64_t>(mem->stats->avail),
             .used      = static_cast<int64_t>(mem->stats->used),
             .max       = static_cast<int64_t>(mem->stats->max),
             .errors    = mem->stats->err,
             .illegal   = mem->stats->illegal };
}

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

std::unordered_map<std::string, stack::stats_data> lwip::stats() const
{
    std::unordered_map<std::string, stack::stats_data> stats;

#if SYS_STATS
    stats.emplace("sems",    make_element_stats_data(&lwip_stats.sys.sem));
    stats.emplace("mutexes", make_element_stats_data(&lwip_stats.sys.mutex));
    stats.emplace("mboxes",  make_element_stats_data(&lwip_stats.sys.mbox));
#endif

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

    return (stats);
}

}
}
}
