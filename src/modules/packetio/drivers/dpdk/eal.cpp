#include <cerrno>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <future>

#include <net/if.h>

#include "lwip/netif.h"
#include "tl/expected.hpp"

#include "packetio/drivers/dpdk/arg_parser.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/eal.hpp"
#include "packetio/drivers/dpdk/mbuf_rx_prbs.hpp"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/mbuf_tx.hpp"
#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "packetio/drivers/dpdk/model/physical_port.hpp"
#include "packetio/generic_port.hpp"
#include "core/op_log.h"
#include "core/op_uuid.hpp"
#include "config/op_config_utils.hpp"

namespace openperf::packetio::dpdk {

/*
 * This file acts as an intermediary between the lower DPDK layer and the
 * upper REST API layer. DPDK knows about ports in terms of a global index
 * in its “known ports” array. The REST API uses ids implemented as strings
 * that the user can define. The eal class handles this mapping. This is why the
 * naming here seems inconsistent at times.
 */

static void
log_port(uint16_t port_idx, std::string_view port_id, model::port_info& info)
{
    struct rte_ether_addr mac_addr;
    rte_eth_macaddr_get(port_idx, &mac_addr);

    if (auto if_index = info.if_index(); if_index > 0) {
        char if_name[IF_NAMESIZE];
        if_indextoname(if_index, if_name);
        OP_LOG(OP_LOG_INFO,
               "Port index %u is using id = %.*s (MAC = "
               "%02x:%02x:%02x:%02x:%02x:%02x, driver = %s attached to %s)",
               port_idx,
               static_cast<int>(port_id.length()),
               port_id.data(),
               mac_addr.addr_bytes[0],
               mac_addr.addr_bytes[1],
               mac_addr.addr_bytes[2],
               mac_addr.addr_bytes[3],
               mac_addr.addr_bytes[4],
               mac_addr.addr_bytes[5],
               info.driver_name(),
               if_name);
    } else {
        OP_LOG(OP_LOG_INFO,
               "Port index %u is using id = %.*s (MAC = "
               "%02x:%02x:%02x:%02x:%02x:%02x, driver = %s)",
               port_idx,
               static_cast<int>(port_id.length()),
               port_id.data(),
               mac_addr.addr_bytes[0],
               mac_addr.addr_bytes[1],
               mac_addr.addr_bytes[2],
               mac_addr.addr_bytes[3],
               mac_addr.addr_bytes[4],
               mac_addr.addr_bytes[5],
               info.driver_name());
    }
}

static int log_link_status_change(uint16_t port_id,
                                  rte_eth_event_type event
                                  __attribute__((unused)),
                                  void* cb_arg __attribute__((unused)),
                                  void* ret_param __attribute__((unused)))
{
    assert(event == RTE_ETH_EVENT_INTR_LSC);
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);
    if (link.link_status == ETH_LINK_UP) {
        OP_LOG(OP_LOG_INFO,
               "Port %u Link Up - speed %u Mbps - %s-duplex\n",
               port_id,
               link.link_speed,
               link.link_duplex == ETH_LINK_FULL_DUPLEX ? "full" : "half");
    } else {
        OP_LOG(OP_LOG_INFO, "Port %u Link Down\n", port_id);
    }

    return (0);
}

__attribute__((const)) static const char* dpdk_logtype(int logtype)
{
    /* Current as of DPDK release 18.08 */
    static const char* logtype_strings[] = {
        "lib.eal",   "lib.malloc",    "lib.ring",  "lib.mempool",
        "lib.timer", "pmd",           "lib.hash",  "lib.lpm",
        "lib.kni",   "lib.acl",       "lib.power", "lib.meter",
        "lib.sched", "lib.port",      "lib.table", "lib.pipeline",
        "lib.mbuf",  "lib.cryptodev", "lib.efd",   "lib.eventdev",
        "lib.gso",   "reserved1",     "reserved2", "reserved3",
        "user1",     "user2",         "user3",     "user4",
        "user5",     "user6",         "user7",     "user8"};

    return ((logtype >= 0
             && logtype < static_cast<int>(op_count_of(logtype_strings)))
                ? logtype_strings[logtype]
                : "unknown");
}

__attribute__((const)) static enum op_log_level dpdk_loglevel(int loglevel)
{
    /*
     * This should be kept in sync with the inferred log levels found in the
     * argument parser.
     */
    switch (loglevel) {
    case RTE_LOG_EMERG:
        return OP_LOG_CRITICAL;
    case RTE_LOG_ALERT:
        return OP_LOG_ERROR;
    case RTE_LOG_CRIT:
        return OP_LOG_WARNING;
    case RTE_LOG_ERR:
        return OP_LOG_INFO;
    case RTE_LOG_WARNING:
    case RTE_LOG_NOTICE:
    case RTE_LOG_INFO:
        return OP_LOG_DEBUG;
    case RTE_LOG_DEBUG:
        return OP_LOG_TRACE;
    default:
        return OP_LOG_NONE;
    }
}

static ssize_t eal_log_write(void* cookie __attribute__((unused)),
                             const char* buf,
                             size_t size)
{
    /*
     * The manpage says size == 0 is an error (by corollary) but dare
     * we not check?
     */
    if (size == 0) return (0);

    /*
     * op_log needs all of the messages to be terminated with a new-line, so fix
     * up any messages that lack such niceties.
     */
    const char* format = (buf[size - 1] == '\n') ? "%.*s" : "%.*s\n";

    /*
     * We can't grab the right function with a macro, so call the
     * actual function and provide the logtype instead.
     */
    op_log(dpdk_loglevel(rte_log_cur_msg_loglevel()),
           dpdk_logtype(rte_log_cur_msg_logtype()),
           format,
           static_cast<int>(size),
           buf);

    return (size);
}

static void
configure_all_ports(const std::map<int, queue::count>& port_queue_counts,
                    const pool_allocator* allocator,
                    const std::unordered_map<int, std::string>& id_name)
{
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV (port_id) {
        auto info = model::port_info(port_id);
        auto mempool = allocator->rx_mempool(info.socket_id());
        auto port = model::physical_port(port_id, id_name.at(port_id), mempool);

        const auto cursor = port_queue_counts.find(port_id);
        if (cursor == port_queue_counts.end()) {
            throw std::runtime_error(
                "No worker threads available for port "
                + std::to_string(port_id)
                + ". Do you need to adjust your core mask?\n");
        }
        const auto& q_count = cursor->second;
        if (auto result = port.low_level_config(q_count.rx, q_count.tx);
            !result) {
            throw std::runtime_error(result.error());
        }
    }
}

static void create_test_portpairs(unsigned test_portpairs)
{
    for (auto i = 0U; i < test_portpairs; i++) {
        std::string ring_name0 = "TSTRNG_" + std::to_string(i * 2);
        auto ring0 = rte_ring_create(
            ring_name0.c_str(), 1024, 0, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring0) {
            throw std::runtime_error(
                "Failed to create ring for vdev eth_ring\n");
        }

        std::string ring_name1 = "TSTRNG_" + std::to_string(i * 2 + 1);
        auto ring1 = rte_ring_create(
            ring_name1.c_str(), 1024, 0, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (!ring1) {
            throw std::runtime_error(
                "Failed to create ring for vdev eth_ring\n");
        }

        if (rte_eth_from_rings(ring_name0.c_str(), &ring0, 1, &ring1, 1, 0)
                == -1
            || rte_eth_from_rings(ring_name1.c_str(), &ring1, 1, &ring0, 1, 0)
                   == -1) {
            throw std::runtime_error("Failed to create vdev eth_ring device\n");
        }
    }
}

static void sanity_check_environment()
{
    if (rte_lcore_count() <= 1) {
        throw std::runtime_error("No DPDK workers cores are available! "
                                 "At least 2 CPU cores are required.");
    }

    const auto misc_mask =
        config::misc_core_mask().value_or(model::core_mask{});
    const auto rx_mask = config::rx_core_mask().value_or(model::core_mask{});
    const auto tx_mask = config::tx_core_mask().value_or(model::core_mask{});
    const auto user_mask = misc_mask | rx_mask | tx_mask;

    if (user_mask[rte_get_master_lcore()]) {
        throw std::runtime_error("User specified mask, "
                                 + model::to_string(user_mask)
                                 + ", conflicts with DPDK master core "
                                 + std::to_string(rte_get_master_lcore()));
    }
}

static void log_idle_workers(const std::vector<queue::descriptor>& descriptors)
{
    /* Generate a mask for all available cores */
    auto eal_mask = model::core_mask{};
    unsigned lcore_id = 0;
    RTE_LCORE_FOREACH_SLAVE (lcore_id) {
        eal_mask.set(lcore_id);
    }
    const auto worker_count = eal_mask.count();

    /* Clear the bits used to support port queues */
    std::for_each(std::begin(descriptors),
                  std::end(descriptors),
                  [&](const auto& d) { eal_mask.reset(d.worker_id); });

    /* Clear the bit used by the stack */
    eal_mask.reset(topology::get_stack_lcore_id());

    /* Warn if we are unable to use any of our available cores */
    if (eal_mask.count()) {
        OP_LOG(OP_LOG_WARNING,
               "Only utilizing %zu of %zu available workers\n",
               worker_count - eal_mask.count(),
               worker_count);
    }
}

eal eal::test_environment(std::vector<std::string>&& args,
                          std::unordered_map<int, std::string>&& port_ids,
                          unsigned test_portpairs)
{
    assert(test_portpairs != 0);
    return (eal(std::forward<decltype(args)>(args),
                std::forward<decltype(port_ids)>(port_ids),
                test_portpairs));
}

eal eal::real_environment(std::vector<std::string>&& args,
                          std::unordered_map<int, std::string>&& port_ids)
{
    return (eal(std::forward<decltype(args)>(args),
                std::forward<decltype(port_ids)>(port_ids)));
}

eal::eal(std::vector<std::string>&& args,
         std::unordered_map<int, std::string>&& port_ids,
         unsigned test_portpairs)
    : m_initialized(false)
    , m_port_ids(port_ids)
{
    /* Convert args to c-strings for DPDK consumption */
    std::vector<char*> eal_args;
    eal_args.reserve(args.size() + 1);
    std::transform(begin(args),
                   end(args),
                   std::back_inserter(eal_args),
                   [](std::string& s) { return s.data(); });
    eal_args.push_back(nullptr); /* null terminator */

    OP_LOG(OP_LOG_INFO,
           "Initializing DPDK with \\\"%s\\\"\n",
           std::accumulate(
               begin(args),
               end(args),
               std::string(),
               [](const std::string& a, const std::string& b) -> std::string {
                   return a + (a.length() > 0 ? " " : "") + b;
               })
               .c_str());

    /* Create a stream for the EAL to use that forwards its messages to our
     * logger */
    cookie_io_functions_t stream_functions = {.write = eal_log_write};

    auto stream = fopencookie(nullptr, "w+", stream_functions);
    if (!stream || rte_openlog_stream(stream) < 0) {
        throw std::runtime_error("Failed to set DPDK log stream");
    }

    /* Initialize the EAL in a separate thread, because an initialization code
     * changes the current thread affinity mask
     */
    int parsed_or_err = std::async(rte_eal_init,
                                   static_cast<int>(eal_args.size() - 1),
                                   eal_args.data())
                            .get();
    if (parsed_or_err < 0) {
        throw std::runtime_error("Failed to initialize DPDK");
    }

    /*
     * rte_eal_init returns the number of parsed arguments; warn if some
     * arguments were unparsed.  We subtract two to account for the trailing
     * null and the program name.
     */
    if (parsed_or_err != static_cast<int>(eal_args.size() - 2)) {
        OP_LOG(OP_LOG_ERROR,
               "DPDK initialization routine only parsed %d of %" PRIu64
               " arguments\n",
               parsed_or_err,
               eal_args.size() - 2);
    }

    sanity_check_environment();

    /* Find some space for Spirent test signatures and PRBS data */
    mbuf_signature_init();
    mbuf_rx_prbs_init();
    mbuf_tx_init();

    if (test_portpairs > 0) { create_test_portpairs(test_portpairs); }

    /*
     * Loop through our available ports and...
     * 1. Log the MAC/driver to the console
     * 2. Generate a port_info vector
     * 3. Generate a UUID name for any port that doesn't already have a name
     */
    uint16_t port_id = 0;
    std::vector<model::port_info> port_info;
    RTE_ETH_FOREACH_DEV (port_id) {
        if (m_port_ids.find(port_id) == m_port_ids.end()) {
            m_port_ids[port_id] = core::to_string(core::uuid::random());
        }
        port_info.emplace_back(model::port_info(port_id));
        log_port(port_id, m_port_ids.at(port_id), port_info.back());
    }

    /* Make sure we have some ports to use */
    if (port_info.empty()) {
        throw std::runtime_error("No DPDK ports are available! "
                                 "At least 1 port is required.");
    }

    /* Sanity check all ports have names. */
    assert(port_info.size() == m_port_ids.size());

    OP_LOG(OP_LOG_INFO,
           "DPDK initialized with %" PRIu64 " ports and %u workers\n",
           port_info.size(),
           rte_lcore_count() - 1);

    /*
     * Determine how we should configure port queues based on ports, lcores,
     * and cpu topoology.
     */
    auto q_descriptors = topology::queue_distribute(port_info);
    log_idle_workers(q_descriptors);
    auto q_counts = queue::get_port_queue_counts(q_descriptors);

    /* Use the port_info and queue counts to allocate our default memory pools
     */
    m_allocator = std::make_unique<pool_allocator>(port_info, q_counts);

    /* Use the queue descriptors to configure all of our ports */
    configure_all_ports(q_counts, m_allocator.get(), m_port_ids);

    /* Finally, register a callback to log link status changes and start our
     * ports. */
    if (int error = rte_eth_dev_callback_register(RTE_ETH_ALL,
                                                  RTE_ETH_EVENT_INTR_LSC,
                                                  log_link_status_change,
                                                  nullptr);
        error != 0) {
        OP_LOG(OP_LOG_WARNING,
               "Could not register link status change callback: %s\n",
               strerror(std::abs(error)));
    }

    m_initialized = true;
}

eal::~eal()
{
    if (!m_initialized) { return; }

    stop_all_ports();

    rte_eth_dev_callback_unregister(
        RTE_ETH_ALL, RTE_ETH_EVENT_INTR_LSC, log_link_status_change, nullptr);

    /* Shut up clang's warning about this being an unstable ABI function */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    rte_eal_cleanup();
#pragma clang diagnostic pop
};

eal::eal(eal&& other) noexcept
    : m_initialized(other.m_initialized)
    , m_allocator(std::move(other.m_allocator))
    , m_bond_ports(std::move(other.m_bond_ports))
    , m_port_ids(std::move(other.m_port_ids))
{
    other.m_initialized = false;
}

eal& eal::operator=(eal&& other) noexcept
{
    if (this != &other) {
        m_initialized = other.m_initialized;
        other.m_initialized = false;
        m_allocator = std::move(other.m_allocator);
        m_bond_ports = std::move(other.m_bond_ports);
        m_port_ids = std::move(other.m_port_ids);
    }
    return (*this);
}

void eal::start_all_ports() const
{
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV (port_id) {
        model::physical_port(port_id, m_port_ids.at(port_id)).start();
    }
}

void eal::stop_all_ports() const
{
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV (port_id) {
        model::physical_port(port_id, m_port_ids.at(port_id)).stop();
    }
}

std::vector<std::string> eal::port_ids() const
{
    uint16_t port_idx = 0;
    std::vector<std::string> port_ids;
    RTE_ETH_FOREACH_DEV (port_idx) {
        port_ids.emplace_back(m_port_ids.at(port_idx));
    }

    return (port_ids);
}

std::optional<port::generic_port> eal::port(std::string_view id) const
{
    auto idx = port_index(id);
    if (!idx) { return (std::nullopt); }

    return (
        rte_eth_dev_is_valid_port(idx.value())
            ? std::make_optional(port::generic_port(model::physical_port(
                idx.value(),
                std::string(id),
                m_allocator->rx_mempool(rte_eth_dev_socket_id(idx.value())))))
            : std::nullopt);
}

/* Method to look up a DPDK port index by REST API id. */
std::optional<int> eal::port_index(std::string_view id) const
{
    auto it = std::find_if(m_port_ids.begin(),
                           m_port_ids.end(),
                           [id](const std::pair<int, std::string>& val) {
                               return val.second == id;
                           });

    return (it != m_port_ids.end() ? std::make_optional(it->first)
                                   : std::nullopt);
}

tl::expected<std::string, std::string>
eal::create_port(std::string_view id, const port::config_data& config)
{
    static unsigned bond_idx = 0;
    /* Sanity check input */
    if (!std::holds_alternative<port::bond_config>(config)) {
        return tl::make_unexpected("Missing bond configuration data");
    }

    std::vector<int> port_indexes;
    /* Make sure that all ports in the vector actually exist */
    for (const auto& port_id : std::get<port::bond_config>(config).ports) {
        // Verify port id is valid.
        auto id_check = openperf::config::op_config_validate_id_string(port_id);
        if (!id_check) { return tl::make_unexpected(id_check.error()); }

        auto idx = port_index(port_id);
        if (!idx || !rte_eth_dev_is_valid_port(idx.value())) {
            return tl::make_unexpected("Port id " + std::string(id)
                                       + " is invalid");
        }
        port_indexes.push_back(idx.value());
    }

    /* Well all right.  Let's create a port, shall we? */
    /*
     * XXX: DPDK uses the prefix of the name to find the proper driver;
     * ergo, this name cannot change.
     */
    std::string name = "net_bonding" + std::to_string(bond_idx++);
    /**
     * Use the port ids to figure out the most common socket.  We'll use that
     * as the socket id for the bonded port.
     */
    int id_or_error =
        rte_eth_bond_create(name.c_str(),
                            BONDING_MODE_8023AD,
                            topology::get_most_common_numa_node(port_indexes));
    if (id_or_error < 0) {
        return tl::make_unexpected(
            "Failed to create bond port: "
            + std::string(rte_strerror(std::abs(id_or_error))));
    }

    std::vector<int> success_record;
    for (auto port_idx : port_indexes) {
        int error = rte_eth_bond_slave_add(id_or_error, port_idx);
        if (error) {
            for (auto added_id : success_record) {
                rte_eth_bond_slave_remove(id_or_error, added_id);
            }
            rte_eth_bond_free(name.c_str());
            return tl::make_unexpected(
                "Failed to add slave port " + std::to_string(port_idx)
                + "to bond port " + std::to_string(id_or_error) + ": "
                + std::string(rte_strerror(std::abs(error))));
        }
        success_record.push_back(port_idx);
    }

    m_bond_ports[id_or_error] = name;
    m_port_ids[id_or_error] = id;
    return (std::string(id));
}

tl::expected<void, std::string> eal::delete_port(std::string_view id)
{
    auto idx = port_index(id);
    if (!idx || !rte_eth_dev_is_valid_port(*idx)) {
        /* Deleting a non-existent port is fine */
        return {};
    }

    /* Port exists */
    auto port_idx = *idx;
    if (m_bond_ports.find(port_idx) == m_bond_ports.end()) {
        /* but it's not one we can delete */
        return tl::unexpected("Port " + std::string(id) + " cannot be deleted");
    }

    /*
     * There is apparently no way to query the number of slaves a port has,
     * so resort to brute force here.
     */
    std::array<uint16_t, RTE_MAX_ETHPORTS> slaves;
    int length_or_err =
        rte_eth_bond_slaves_get(port_idx, slaves.data(), slaves.size());
    if (length_or_err < 0) {
        /* Not sure what else we can do here... */
        OP_LOG(OP_LOG_ERROR,
               "Could not retrieve slave port ids from bonded port %s\n",
               id.data());
    } else if (length_or_err > 0) {
        for (int i = 0; i < length_or_err; i++) {
            rte_eth_bond_slave_remove(port_idx, slaves[i]);
        }
    }
    rte_eth_bond_free(m_bond_ports[port_idx].c_str());
    m_bond_ports.erase(port_idx);
    m_port_ids.erase(port_idx);
    return {};
}

} // namespace openperf::packetio::dpdk
