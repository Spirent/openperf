#include <atomic>
#include <sys/timerfd.h>

#include "lwipopts.h"
#include "lwip/tcpip.h"

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/topology_utils.h"
#include "packetio/stack/dpdk/netif_wrapper.h"
#include "packetio/stack/dpdk/lwip.h"
#include "packetio/stack/dpdk/sys_mailbox.h"
#include "packetio/stack/tcpip.h"

sys_mbox_t icp::packetio::tcpip::mbox()
{
    static auto tcpip_mbox = sys_mbox();
    return (std::addressof(tcpip_mbox));
}

namespace icp::packetio::dpdk {

#define STRING_VIEW_TO_C_STR(sv) static_cast<int>(sv.length()), sv.data()

static void tcpip_init_done(void *arg __attribute__((unused)))
{
    ICP_LOG(ICP_LOG_DEBUG, "TCP/IP thread running on logical core %u (NUMA node %u)\n",
            rte_lcore_id(), rte_socket_id());
}

static constexpr timespec duration_to_timespec(std::chrono::milliseconds ms)
{
    return (timespec {
            .tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(ms).count(),
            .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(ms).count() });
}

static int handle_tcpip_timeout(event_loop::generic_event_loop& loop __attribute__((unused)),
                                std::any arg)
{
    auto timer_fd = std::any_cast<int>(arg);

    uint64_t count;
    if (read(timer_fd, &count, sizeof(count)) == -1) {
        ICP_LOG(ICP_LOG_WARNING, "Spurious stack thread timeout\n");
    }

    auto sleeptime = tcpip::handle_timeouts();
    auto wake_up = itimerspec {
        .it_interval = { 0, 0 },
        .it_value = duration_to_timespec(sleeptime)
    };

    return (timerfd_settime(timer_fd, 0, &wake_up, nullptr));
}

static int handle_tcpip_message(event_loop::generic_event_loop& loop __attribute__((unused)),
                               std::any arg)
{
    auto mbox = std::any_cast<sys_mbox_t>(arg);
    return (tcpip::handle_messages(mbox));
}

lwip::lwip(driver::generic_driver& driver, workers::generic_workers& workers)
    : m_driver(driver)
    , m_workers(workers)
    , m_timerfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK))
{
    if (m_timerfd == -1) {
        throw std::runtime_error("Could not create kernel timer: "
                                 + std::string(strerror(errno)));
    }

    auto tcpip_mbox = tcpip::mbox();
    auto msg_id = m_workers.add_task(workers::context::STACK,
                                     "stack_message_handler",
                                     sys_mbox_fd(&tcpip_mbox),
                                     handle_tcpip_message,
                                     tcpip_mbox);
    if (!msg_id) {
        throw std::runtime_error("Could not create message handler task: "
                                 + std::string(strerror(msg_id.error())));
    }
    m_tasks.push_back(*msg_id);

    auto timeout_id = m_workers.add_task(workers::context::STACK,
                                         "stack_timeout_handler",
                                         m_timerfd,
                                         handle_tcpip_timeout,
                                         m_timerfd);
    if (!timeout_id) {
        throw std::runtime_error("Could not create timeout handler task: "
                                 + std::string(strerror(msg_id.error())));
    }
    m_tasks.push_back(*timeout_id);

    if (tcpip_callback(tcpip_init_done, nullptr) != ERR_OK) {
        throw std::runtime_error("Could not initialize stack");
    }

    m_initialized = true;
}

lwip::~lwip()
{
    if (!m_initialized) return;

    for (auto& id : m_tasks) {
        m_workers.del_task(id);
    }

    close(m_timerfd);
}

lwip& lwip::operator=(lwip&& other)
{
    if (this != &other) {
        m_initialized = other.m_initialized;
        other.m_initialized = false;

        m_driver = std::move(other.m_driver);
        m_workers = std::move(other.m_workers);
        m_tasks = std::move(other.m_tasks);
        m_interfaces = std::move(other.m_interfaces);

        m_timerfd = other.m_timerfd;
    }
    return (*this);
}

lwip::lwip(lwip&& other)
    : m_initialized(other.m_initialized)
    , m_driver(other.m_driver)
    , m_workers(other.m_workers)
    , m_tasks(std::move(other.m_tasks))
    , m_interfaces(std::move(other.m_interfaces))
    , m_timerfd(other.m_timerfd)
{
    other.m_initialized = false;
}

std::vector<std::string> lwip::interface_ids() const
{
    std::vector<std::string> ids;
    for (auto& item : m_interfaces) {
        ids.push_back(item.first);
    }

    return (ids);
}

static netif_wrapper make_netif_wrapper(const std::unique_ptr<net_interface>& ifp)
{
    return (netif_wrapper(ifp->id(), ifp->data(), ifp->config()));
}

std::optional<interface::generic_interface> lwip::interface(std::string_view id) const
{
    return (m_interfaces.find(id) != m_interfaces.end()
            ? std::make_optional(interface::generic_interface(
                                 make_netif_wrapper(m_interfaces.at(std::string(id)))))
            : std::nullopt);
}

tl::expected<std::string, std::string> lwip::create_interface(const interface::config_data& config)
{
    // Check for a duplicate interface name.
    if (m_interfaces.count(config.id))
        return (tl::make_unexpected("Interface " + config.id + " already exists."));

    try {
        auto port_index = m_driver.port_index(config.port_id);
        if (!port_index) {
            return (tl::make_unexpected("Port id " + config.port_id + " is invalid."));
        }
        int port_idx = port_index.value();
        auto ifp = std::make_unique<net_interface>(config.id, config,
                                                   m_workers.get_transmit_function(config.port_id),
                                                   port_idx);
        m_workers.add_interface(ifp->port_id(), ifp->data());
        auto item = m_interfaces.emplace(std::make_pair(config.id, std::move(ifp)));
        return (item.first->first);
    } catch (const std::runtime_error &e) {
        ICP_LOG(ICP_LOG_ERROR, "Interface creation failed: %s\n", e.what());
        return (tl::make_unexpected(e.what()));
    }
}

void lwip::delete_interface(std::string_view id)
{
    if (auto item = m_interfaces.find(id); item != m_interfaces.end()) {
        auto& ifp = item->second;
        (void)ifp;
        m_workers.del_interface(ifp->port_id(), std::make_any<netif*>(ifp->data()));
        m_interfaces.erase(std::string(id));
    }
}

tl::expected<void, int> lwip::attach_interface_sink(std::string_view id,
                                                    pga::generic_sink& sink)
{
    auto item = m_interfaces.find(id);
    if (item == m_interfaces.end()) {
        return (tl::make_unexpected(ENODEV));
    }

    auto& ifp = item->second;
    if (auto error = ifp->attach_sink(sink)) {
        return (tl::make_unexpected(error));
    }

    ICP_LOG(ICP_LOG_DEBUG, "Attached sink %s to interface %.*s\n",
            sink.id().c_str(), STRING_VIEW_TO_C_STR(id));

    return {};
}

void lwip::detach_interface_sink(std::string_view id, pga::generic_sink& sink)
{
    if (auto item = m_interfaces.find(id); item != m_interfaces.end()) {
        auto& ifp = item->second;
        ifp->detach_sink(sink);

        ICP_LOG(ICP_LOG_DEBUG, "Detached sink %s from interface %.*s\n",
                sink.id().c_str(), STRING_VIEW_TO_C_STR(id));
    }
}

tl::expected<void, int> lwip::attach_interface_source(std::string_view id,
                                                      pga::generic_source& source)
{
    auto item = m_interfaces.find(id);
    if (item == m_interfaces.end()) {
        return (tl::make_unexpected(ENODEV));
    }

    auto& ifp = item->second;
    if (auto error = ifp->attach_source(source)) {
        return (tl::make_unexpected(error));
    }

    ICP_LOG(ICP_LOG_DEBUG, "Attached source %s to interface %.*s\n",
            source.id().c_str(), STRING_VIEW_TO_C_STR(id));

    return {};
}

void lwip::detach_interface_source(std::string_view id, pga::generic_source& source)
{
    if (auto item = m_interfaces.find(id); item != m_interfaces.end()) {
        auto& ifp = item->second;
        ifp->detach_source(source);

        ICP_LOG(ICP_LOG_DEBUG, "Detached source %s from interface %.*s\n",
                source.id().c_str(), STRING_VIEW_TO_C_STR(id));
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
