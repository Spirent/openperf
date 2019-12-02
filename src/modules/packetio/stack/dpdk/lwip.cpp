#include <atomic>
#include <sys/timerfd.h>

#include "lwipopts.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"

#include "core/op_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "packetio/memory/dpdk/memp.h"
#include "packetio/stack/dpdk/lwip.hpp"
#include "packetio/stack/dpdk/tcpip_mbox.hpp"
#include "packetio/stack/netif_wrapper.hpp"
#include "packetio/stack/tcpip.hpp"

namespace openperf::packetio::dpdk {

static netif_wrapper
make_netif_wrapper(const std::unique_ptr<net_interface>& ifp)
{
    return (netif_wrapper(ifp->id(), ifp->data(), ifp->config()));
}

static void tcpip_init_done(void*)
{
    OP_LOG(OP_LOG_DEBUG,
           "TCP/IP thread running on logical core %u (NUMA node %u)\n",
           rte_lcore_id(),
           rte_socket_id());
}

static constexpr timespec duration_to_timespec(std::chrono::milliseconds ms)
{
    return (timespec{
        .tv_sec = std::chrono::duration_cast<std::chrono::seconds>(ms).count(),
        .tv_nsec =
            std::chrono::duration_cast<std::chrono::nanoseconds>(ms).count()});
}

static int handle_tcpip_timeout(event_loop::generic_event_loop& loop
                                __attribute__((unused)),
                                std::any arg)
{
    auto timer_fd = std::any_cast<int>(arg);

    uint64_t count;
    if (read(timer_fd, &count, sizeof(count)) == -1) {
        OP_LOG(OP_LOG_WARNING, "Spurious stack thread timeout\n");
        return (0);
    }

    auto sleeptime = tcpip::handle_timeouts();
    auto wake_up = itimerspec{.it_interval = {0, 0},
                              .it_value = duration_to_timespec(sleeptime)};

    return (timerfd_settime(timer_fd, 0, &wake_up, nullptr));
}

static int handle_tcpip_message(event_loop::generic_event_loop& loop
                                __attribute__((unused)),
                                std::any arg)
{
    auto mbox = std::any_cast<sys_mbox_t>(arg);
    return (tcpip::handle_messages(mbox));
}

/*
 * DPDK callback for notifying interfaces about link state changes.
 * Having 1 callback per port seems like a better idea than setting up 1
 * callback per interface.
 */
static int lwip_link_status_change_callback(uint16_t port_id,
                                            rte_eth_event_type event
                                            __attribute__((unused)),
                                            void* cb_arg,
                                            void* ret_param
                                            __attribute__((unused)))
{
    assert(event == RTE_ETH_EVENT_INTR_LSC);
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);

    auto& map = *(reinterpret_cast<lwip::interface_map*>(cb_arg));

    std::for_each(std::begin(map), std::end(map), [&](auto& pair) {
        if (pair.second->port_index() == port_id) {
            pair.second->handle_link_state_change(link.link_status
                                                  == ETH_LINK_UP);
        }
    });

    return (0);
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

    lwip_init();

    /*
     * XXX: lwIP uses a global sys_mbox_t to allow communication between
     * clients and the stack thread.  We initialize this object here and
     * de-initialize it in our destructor.  This coupling is necessary to
     * allow the process to cleanly shut down.  Luckily, client functions
     * check for a valid mbox object before using it.  But if we don't
     * invalidate it, they can block on a callback waiting for a response
     * from our non-existent stack tasks.
     */
    auto tcpip_mbox = tcpip_mbox::instance().init();
    auto msg_id = m_workers.add_task(workers::context::STACK,
                                     "stack API messages",
                                     sys_mbox_fd(&tcpip_mbox),
                                     handle_tcpip_message,
                                     tcpip_mbox);
    if (!msg_id) {
        throw std::runtime_error("Could not create message handler task: "
                                 + std::string(strerror(msg_id.error())));
    }
    m_tasks.push_back(*msg_id);

    auto timeout_id = m_workers.add_task(workers::context::STACK,
                                         "stack timers",
                                         m_timerfd,
                                         handle_tcpip_timeout,
                                         m_timerfd);
    if (!timeout_id) {
        throw std::runtime_error("Could not create timeout handler task: "
                                 + std::string(strerror(msg_id.error())));
    }
    m_tasks.push_back(*timeout_id);

    /* Kick the stack timer to get it started */
    auto kick = itimerspec{.it_interval = {0, 0}, .it_value = {0, 100}};

    if (timerfd_settime(m_timerfd, 0, &kick, nullptr) == -1) {
        throw std::runtime_error("Could not set initial TCPIP stack timeout: "
                                 + std::string(strerror(errno)));
    }

    /* Run a simple callback in the stack to log that it has started */
    if (tcpip_callback(tcpip_init_done, nullptr) != ERR_OK) {
        throw std::runtime_error("Could not initialize stack");
    }

    /* Register for link status change callbacks for all DPDK ports */
    if (rte_eth_dev_callback_register(RTE_ETH_ALL,
                                      RTE_ETH_EVENT_INTR_LSC,
                                      lwip_link_status_change_callback,
                                      std::addressof(m_interfaces))) {
        throw std::runtime_error(
            "Could not register DPDK link status change callback");
    }

    m_initialized = true;
}

lwip::~lwip()
{
    if (!m_initialized) return;

    for (auto& id : m_tasks) { m_workers.del_task(id); }

    tcpip_mbox::instance().fini();
    close(m_timerfd);

    rte_eth_dev_callback_unregister(RTE_ETH_ALL,
                                    RTE_ETH_EVENT_INTR_LSC,
                                    lwip_link_status_change_callback,
                                    std::addressof(m_interfaces));

    m_interfaces.clear();
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

    if (rte_eth_dev_callback_unregister(RTE_ETH_ALL,
                                        RTE_ETH_EVENT_INTR_LSC,
                                        lwip_link_status_change_callback,
                                        std::addressof(other.m_interfaces))
        || rte_eth_dev_callback_register(RTE_ETH_ALL,
                                         RTE_ETH_EVENT_INTR_LSC,
                                         lwip_link_status_change_callback,
                                         std::addressof(m_interfaces))) {
        OP_LOG(OP_LOG_ERROR,
               "Could not update DPDK link status change callback\n");
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
    if (rte_eth_dev_callback_unregister(RTE_ETH_ALL,
                                        RTE_ETH_EVENT_INTR_LSC,
                                        lwip_link_status_change_callback,
                                        std::addressof(other.m_interfaces))
        || rte_eth_dev_callback_register(RTE_ETH_ALL,
                                         RTE_ETH_EVENT_INTR_LSC,
                                         lwip_link_status_change_callback,
                                         std::addressof(m_interfaces))) {
        OP_LOG(OP_LOG_ERROR,
               "Could not update DPDK link status change callback\n");
    }

    other.m_initialized = false;
}

std::vector<std::string> lwip::interface_ids() const
{
    std::vector<std::string> ids;
    std::transform(std::begin(m_interfaces),
                   std::end(m_interfaces),
                   std::back_inserter(ids),
                   [](auto& item) { return (item.first); });
    return (ids);
}

std::optional<interface::generic_interface>
lwip::interface(std::string_view id) const
{
    return (m_interfaces.find(id) != m_interfaces.end()
                ? std::make_optional(interface::generic_interface(
                    make_netif_wrapper(m_interfaces.at(std::string(id)))))
                : std::nullopt);
}

tl::expected<std::string, std::string>
lwip::create_interface(const interface::config_data& config)
{
    // Check for a duplicate interface name.
    if (m_interfaces.count(config.id))
        return (
            tl::make_unexpected("Interface " + config.id + " already exists."));

    try {
        auto port_index = m_driver.port_index(config.port_id);
        if (!port_index) {
            return (tl::make_unexpected("Port id " + config.port_id
                                        + " is invalid."));
        }
        int port_idx = port_index.value();
        auto ifp = std::make_unique<net_interface>(
            config.id,
            port_idx,
            config,
            m_workers.get_transmit_function(config.port_id));
        m_workers.add_interface(
            ifp->port_id(),
            interface::generic_interface(make_netif_wrapper(ifp)));
        auto item = m_interfaces.emplace(config.id, std::move(ifp));
        return (item.first->first);
    } catch (const std::runtime_error& e) {
        OP_LOG(OP_LOG_ERROR, "Interface creation failed: %s\n", e.what());
        return (tl::make_unexpected(e.what()));
    }
}

void lwip::delete_interface(std::string_view id)
{
    if (auto item = m_interfaces.find(id); item != m_interfaces.end()) {
        auto& ifp = item->second;
        m_workers.del_interface(
            ifp->port_id(),
            interface::generic_interface(make_netif_wrapper(ifp)));
        m_interfaces.erase(std::string(id));
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
static stack::element_stats_data
make_element_stats_data(const stats_syselem* syselem)
{
    return {.used = syselem->used, .max = syselem->max, .errors = syselem->err};
}
#endif

#if MEMP_STATS
static stack::memory_stats_data make_memory_stats_data(const memp_desc* mem)
{
    return {.name = mem->desc,
            .available = packetio_memory_memp_pool_avail(mem),
            .used = packetio_memory_memp_pool_used(mem),
            .max = packetio_memory_memp_pool_max(mem),
            .errors = mem->stats->err,
            .illegal = mem->stats->illegal};
}
#endif

#if IGMP_STATS || MLD6_STATS
static stack::protocol_stats_data
make_protocol_stats_data(const stats_igmp* igmp)
{
    return {.tx_packets = igmp->xmit,
            .rx_packets = igmp->recv,
            .dropped_packets = igmp->drop,
            .checksum_errors = igmp->chkerr,
            .length_errors = igmp->lenerr,
            .memory_errors = igmp->memerr,
            .protocol_errors = igmp->proterr};
}
#endif

static stack::protocol_stats_data
make_protocol_stats_data(const stats_proto* proto)
{
    return {.tx_packets = proto->xmit,
            .rx_packets = proto->recv,
            .forwarded_packets = proto->fw,
            .dropped_packets = proto->drop,
            .checksum_errors = proto->chkerr,
            .length_errors = proto->lenerr,
            .memory_errors = proto->memerr,
            .routing_errors = proto->rterr,
            .protocol_errors = proto->proterr,
            .option_errors = proto->opterr,
            .misc_errors = proto->err,
            .cache_hits = proto->cachehit};
}
#endif /* LWIP_STATS */

std::unordered_map<std::string, stack::stats_data> lwip::stats() const
{
    std::unordered_map<std::string, stack::stats_data> stats;

#if LWIP_STATS
#if SYS_STATS
    stats.emplace("sems", make_element_stats_data(&lwip_stats.sys.sem));
    stats.emplace("mutexes", make_element_stats_data(&lwip_stats.sys.mutex));
    stats.emplace("mboxes", make_element_stats_data(&lwip_stats.sys.mbox));
#endif

#if MEMP_STATS
    /**
     * The mem_std.h file contains a LWIP_MEMPOOL() entry for every mempool
     * used by the stack.  Additionally, those mempool/macro blocks are
     * all guarded by ifdefs, so they always match the actual LwIP
     * compilation options.  Hence, we use the following macro + #include
     * to generate our memory statistics.
     */
#define LWIP_MEMPOOL(pool_name, ...)                                           \
    stats.emplace(memp_pools[MEMP_##pool_name]->desc,                          \
                  make_memory_stats_data(memp_pools[MEMP_##pool_name]));
#include "lwip/priv/memp_std.h"
#endif

#if ETHARP_STATS
    stats.emplace("arp", make_protocol_stats_data(&lwip_stats.etharp));
#endif

#if IP_STATS
    stats.emplace("ipv4", make_protocol_stats_data(&lwip_stats.ip));
#endif

#if IPFRAG_STATS
    stats.emplace("ipv4_frag", make_protocol_stats_data(&lwip_stats.ip_frag));
#endif

#if ICMP_STATS
    stats.emplace("icmpv4", make_protocol_stats_data(&lwip_stats.icmp));
#endif

#if IGMP_STATS
    stats.emplace("igmp", make_protocol_stats_data(&lwip_stats.igmp));
#endif

#if ND6_STATS
    stats.emplace("nd", make_protocol_stats_data(&lwip_stats.nd6));
#endif

#if IP6_STATS
    stats.emplace("ipv6", make_protocol_stats_data(&lwip_stats.ip6));
#endif

#if IP6_FRAG_STATS
    stats.emplace("ipv6_frag", make_protocol_stats_data(&lwip_stats.ip6_frag));
#endif

#if ICMP6_STATS
    stats.emplace("icmpv6", make_protocol_stats_data(&lwip_stats.icmp6));
#endif

#if MLD6_STATS
    stats.emplace("mld", make_protocol_stats_data(&lwip_stats.mld6));
#endif

#if UDP_STATS
    stats.emplace("udp", make_protocol_stats_data(&lwip_stats.udp));
#endif

#if TCP_STATS
    stats.emplace("tcp", make_protocol_stats_data(&lwip_stats.tcp));
#endif
#endif /* LWIP_STATS */

    return (stats);
}

} // namespace openperf::packetio::dpdk
