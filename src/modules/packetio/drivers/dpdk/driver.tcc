#include <map>
#include <optional>
#include <vector>

#include <sched.h>

#include "config/op_config_utils.hpp"
#include "core/op_cpuset.h"
#include "core/op_log.h"
#include "core/op_thread.h"
#include "core/op_uuid.hpp"
#include "packetio/drivers/dpdk/arg_parser.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/driver.hpp"
#include "packetio/drivers/dpdk/mbuf_rx_prbs.hpp"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/mbuf_tx.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "packetio/generic_port.hpp"

namespace openperf::packetio::dpdk {

/***
 * Various DPDK static functions
 ***/

static inline void log_port(uint16_t idx, std::string_view id)
{
    if (auto if_name = port_info::interface_name(idx)) {
        OP_LOG(OP_LOG_INFO,
               "Port index %u is using id = %.*s (MAC = %s, "
               "driver = %s attached to %s)",
               idx,
               static_cast<int>(id.length()),
               id.data(),
               libpacket::type::to_string(port_info::mac_address(idx)).c_str(),
               port_info::driver_name(idx).c_str(),
               if_name.value().c_str());
    } else {
        OP_LOG(OP_LOG_INFO,
               "Port index %u is using id = %.*s (MAC = %s, driver = %s)",
               idx,
               static_cast<int>(id.length()),
               id.data(),
               libpacket::type::to_string(port_info::mac_address(idx)).c_str(),
               port_info::driver_name(idx).c_str());
    }
}

static inline void sanity_check_environment()
{
    if (rte_lcore_count() <= 1) {
        throw std::runtime_error("No DPDK worker cores are available! "
                                 "At least 2 CPU cores are required.");
    }

    const auto misc_mask = config::misc_core_mask().value_or(core::cpuset{});
    const auto rx_mask = config::rx_core_mask().value_or(core::cpuset{});
    const auto tx_mask = config::tx_core_mask().value_or(core::cpuset{});
    const auto user_mask = misc_mask | rx_mask | tx_mask;

    if (user_mask[rte_get_master_lcore()]) {
        throw std::runtime_error("User specified mask, " + user_mask.to_string()
                                 + ", conflicts with DPDK master core "
                                 + std::to_string(rte_get_master_lcore()));
    }
}

template <typename T, typename... Things>
constexpr auto array(Things&&... things) -> std::array<T, sizeof...(things)>
{
    return {{std::forward<Things>(things)...}};
}

constexpr const char* dpdk_logtype(int logtype)
{
    /* Current as of DPDK release 18.08 */
    constexpr auto logtype_strings = array<std::string_view>("lib.eal",
                                                             "lib.malloc",
                                                             "lib.ring",
                                                             "lib.mempool",
                                                             "lib.timer",
                                                             "pmd",
                                                             "lib.hash",
                                                             "lib.lpm",
                                                             "lib.kni",
                                                             "lib.acl",
                                                             "lib.power",
                                                             "lib.meter",
                                                             "lib.sched",
                                                             "lib.port",
                                                             "lib.table",
                                                             "lib.pipeline",
                                                             "lib.mbuf",
                                                             "lib.cryptodev",
                                                             "lib.efd",
                                                             "lib.eventdev",
                                                             "lib.gso",
                                                             "reserved1",
                                                             "reserved2",
                                                             "reserved3",
                                                             "user1",
                                                             "user2",
                                                             "user3",
                                                             "user4",
                                                             "user5",
                                                             "user6",
                                                             "user7",
                                                             "user8");

    return (logtype >= 0 && logtype < static_cast<int>(logtype_strings.size())
                ? logtype_strings[logtype].data()
                : "unknown");
}

constexpr enum op_log_level dpdk_loglevel(int loglevel)
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
     * op_log needs all of the messages to be terminated with a new-line, so
     * fix up any messages that lack such niceties.
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

/*
 * Perform the minimal amount of work necessary to load the DPDK environment
 * After this is run, we'll hand off to process specific code.
 */
static void bootstrap()
{
    /*
     * Create a stream so we can forward DPDK logging messages to our own
     * internal logger.
     */
    cookie_io_functions_t stream_functions = {.write = eal_log_write};
    auto stream = fopencookie(nullptr, "w+", stream_functions);
    if (!stream || rte_openlog_stream(stream) < 0) {
        throw std::runtime_error("Failed to set DPDK log stream");
    }

    /* Convert args to c-strings for DPDK consumption */
    auto args = config::dpdk_args();
    auto eal_args = std::vector<char*>{};
    eal_args.reserve(args.size() + 1);
    std::transform(std::begin(args),
                   std::end(args),
                   std::back_inserter(eal_args),
                   [](auto& s) { return s.data(); });
    eal_args.emplace_back(nullptr); /* null terminator */

    OP_LOG(OP_LOG_INFO,
           "Initializing DPDK with \\\"%s\\\"\n",
           std::accumulate(std::begin(args),
                           std::end(args),
                           std::string{},
                           [](std::string& lhs, const std::string& rhs) {
                               return (lhs += (lhs.length() ? " " : "") + rhs);
                           })
               .c_str());

    auto parsed_or_err =
        rte_eal_init(static_cast<int>(eal_args.size() - 1), eal_args.data());
    if (parsed_or_err < 0) {
        throw std::runtime_error("Failed to initialize DPDK: "
                                 + std::string(rte_strerror(rte_errno)));
    }

    /*
     * rte_eal_init returns the number of parsed arguments; warn if some
     * arguments were unparsed. We subtract two to account for the trailing
     * null and the program name.
     */
    if (parsed_or_err != static_cast<int>(eal_args.size() - 2)) {
        OP_LOG(OP_LOG_ERROR,
               "DPDK initialization function only parsed %d of %" PRIu64
               " arguments\n",
               parsed_or_err,
               eal_args.size() - 2);
    }
}

using cpuset_type = std::remove_pointer_t<op_cpuset_t>;

struct cpuset_deleter
{
    void operator()(cpuset_type* cpuset) const { op_cpuset_delete(cpuset); }
};

using cpuset_ptr = std::unique_ptr<cpuset_type, cpuset_deleter>;

static void do_bootstrap()
{
    /*
     * The DPDK init process locks this thread to the first
     * available DPDK core.  However, since this thread is going
     * to launch other threads, we need to restore the original
     * thread affinity mask.
     */
    auto cpuset = cpuset_ptr{op_cpuset_create()};
    if (auto error = op_thread_get_affinity_mask(cpuset.get())) {
        throw std::runtime_error("Could not retrieve CPU affinity mask: "
                                 + std::string(strerror(error)));
    }

    bootstrap();

    /* Remove all DPDK slave cores from our original affinity mask */
    int lcore_id = 0;
    RTE_LCORE_FOREACH_SLAVE (lcore_id) {
        op_cpuset_set(cpuset.get(), lcore_id, false);
    }

    /* Now restore the original cpu mask less the DPDK slave cores */
    if (auto error = op_thread_set_affinity_mask(cpuset.get())) {
        throw std::runtime_error("Could not set CPU affinity mask: "
                                 + std::string(strerror(error)));
    }
}

template <typename ProcessType> driver<ProcessType>::driver()
{
    do_bootstrap();

    sanity_check_environment();

    m_process = std::make_unique<ProcessType>();

    /* Generate our map of port ids/indexes */
    auto port_indexes = topology::get_ports();
    auto port_ids = config::dpdk_id_map();

    /* Drop any configured ports we don't know about */
    for (auto it = std::begin(port_ids); it != std::end(port_ids);) {
        if (std::find(
                std::begin(port_indexes), std::end(port_indexes), it->first)
            == std::end(port_indexes)) {
            port_ids.erase(it++);
        } else {
            ++it;
        }
    }

    /* Make sure each available port has a corresponding id */
    for (const auto& idx : port_indexes) {
        if (port_ids.count(idx) == 0) {
            port_ids[idx] = core::to_string(core::uuid::random());
        }
    }

    m_ethdev_ports = port_ids;

    if (m_ethdev_ports.empty()) { return; } /* nothing left to do */

    /* Log port details */
    std::for_each(std::begin(m_ethdev_ports),
                  std::end(m_ethdev_ports),
                  [](const auto& item) { log_port(item.first, item.second); });

    /* Setup mbuf signature/offload flags */
    mbuf_signature_init();
    mbuf_rx_prbs_init();
    mbuf_tx_init();

    /* Finally, start all ports */
    std::for_each(
        std::begin(m_ethdev_ports),
        std::end(m_ethdev_ports),
        [this](const auto& pair) { m_process->start_port(pair.first); });
}

template <typename ProcessType> driver<ProcessType>::~driver()
{
    /* Stop all ports */
    std::for_each(
        std::begin(m_ethdev_ports),
        std::end(m_ethdev_ports),
        [this](const auto& pair) { m_process->stop_port(pair.first); });
}

template <typename ProcessType>
driver<ProcessType>::driver(driver&& other) noexcept
    : m_process(std::move(other.m_process))
    , m_ethdev_ports(std::move(other.m_ethdev_ports))
    , m_bonded_ports(std::move(other.m_bonded_ports))
{}

template <typename ProcessType>
driver<ProcessType>&
driver<ProcessType>::driver::operator=(driver&& other) noexcept
{
    if (this != &other) {
        m_process = std::move(other.m_process);
        m_ethdev_ports = std::move(other.m_ethdev_ports);
        m_bonded_ports = std::move(other.m_bonded_ports);
    }
    return (*this);
}

template <typename ProcessType>
std::vector<std::string> driver<ProcessType>::port_ids() const
{
    auto ids = std::vector<std::string>{};

    std::transform(std::begin(m_ethdev_ports),
                   std::end(m_ethdev_ports),
                   std::back_inserter(ids),
                   [](const auto& item) { return (item.second); });

    return (ids);
}

template <typename ProcessType>
std::optional<port::generic_port>
driver<ProcessType>::port(std::string_view id) const
{
    auto idx = port_index(id);
    if (!idx || !rte_eth_dev_is_valid_port(idx.value())) {
        return (std::nullopt);
    }

    return (m_process->get_port(idx.value(), id));
}

template <typename ProcessType>
std::optional<int> driver<ProcessType>::port_index(std::string_view id) const
{
    if (auto it = std::find_if(
            std::begin(m_ethdev_ports),
            std::end(m_ethdev_ports),
            [id](const auto& pair) { return (pair.second == id); });
        it != std::end(m_ethdev_ports)) {
        return (it->first);
    }

    return (std::nullopt);
}

template <typename ProcessType>
tl::expected<std::string, std::string>
driver<ProcessType>::create_port(std::string_view id,
                                 const port::config_data& config)
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
     * Use the port ids to figure out the most common socket.  We'll use
     * that as the socket id for the bonded port.
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
                + " to bond port " + std::to_string(id_or_error) + ": "
                + std::string(rte_strerror(std::abs(error))));
        }
        success_record.push_back(port_idx);
    }

    m_bonded_ports[id_or_error] = name;
    m_ethdev_ports[id_or_error] = id;
    return (std::string(id));
}

template <typename ProcessType>
tl::expected<void, std::string>
driver<ProcessType>::delete_port(std::string_view id)
{
    auto idx = port_index(id);
    if (!idx || !rte_eth_dev_is_valid_port(*idx)) {
        /* Deleting a non-existent port is fine */
        return {};
    }

    /* Port exists */
    auto port_idx = *idx;
    if (m_bonded_ports.find(port_idx) == m_bonded_ports.end()) {
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
    rte_eth_bond_free(m_bonded_ports[port_idx].c_str());
    m_bonded_ports.erase(port_idx);
    m_ethdev_ports.erase(port_idx);
    return {};
}

template <typename ProcessType> bool driver<ProcessType>::is_usable()
{
    return (!m_ethdev_ports.empty() && rte_lcore_count() > 0);
}

} // namespace openperf::packetio::dpdk
