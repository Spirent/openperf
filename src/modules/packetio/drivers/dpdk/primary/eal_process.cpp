#include "core/op_log.h"
#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "packetio/drivers/dpdk/primary/arg_parser.hpp"
#include "packetio/drivers/dpdk/primary/eal_process.hpp"
#include "packetio/drivers/dpdk/primary/physical_port.hpp"
#include "packetio/drivers/dpdk/primary/utils.hpp"

namespace openperf::packetio::dpdk::primary {

namespace detail {

template <typename, typename = std::void_t<>>
struct has_port_setup : std::false_type
{};

template <typename T>
struct has_port_setup<T,
                      std::void_t<decltype(std::declval<T>().do_port_setup())>>
    : std::true_type
{};

template <typename T>
inline constexpr bool has_port_setup_v = has_port_setup<T>::value;

template <typename, typename = std::void_t<>>
struct has_port_callbacks : std::false_type
{};

template <typename T>
struct has_port_callbacks<
    T,
    std::void_t<decltype(std::declval<T>().do_port_callbacks())>>
    : std::true_type
{};

template <typename T>
inline constexpr bool has_port_callbacks_v = has_port_callbacks<T>::value;

template <typename, typename = std::void_t<>>
struct has_cleanup : std::false_type
{};

template <typename T>
struct has_cleanup<T, std::void_t<decltype(std::declval<T>().do_cleanup())>>
    : std::true_type
{};

template <typename T>
inline constexpr bool has_cleanup_v = has_cleanup<T>::value;

} // namespace detail

static void log_idle_workers(const std::vector<queue::descriptor>& descriptors)
{
    /* Generate a mask for all available cores */
    auto eal_mask = core::cpuset{};
    unsigned lcore_id = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) { eal_mask.set(lcore_id); }
    const auto worker_count = eal_mask.count();

    /* Clear the bits used to support port queues */
    std::for_each(std::begin(descriptors),
                  std::end(descriptors),
                  [&](const auto& d) { eal_mask.reset(d.worker_id); });

    /* Clear the bit used by the stack */
    if (auto stack_id = topology::get_stack_lcore_id()) {
        eal_mask.reset(stack_id.value());
    }

    /* Warn if we are unable to use any of our available cores */
    if (eal_mask.count()) {
        OP_LOG(OP_LOG_WARNING,
               "Only utilizing %zu of %zu available workers\n",
               worker_count - eal_mask.count(),
               worker_count);
    }
}

static int log_link_status_change(uint16_t port_id,
                                  [[maybe_unused]] rte_eth_event_type event,
                                  void*,
                                  void*)
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

static void
configure_all_ports(const std::map<uint16_t, queue::count>& port_queue_counts,
                    const pool_allocator& allocator)
{
    assert(rte_eal_process_type() == RTE_PROC_PRIMARY);

    std::for_each(std::begin(port_queue_counts),
                  std::end(port_queue_counts),
                  [&](const auto& pair) {
                      const auto& port_id = pair.first;
                      const auto& q_count = pair.second;

                      auto mempool =
                          allocator.get_mempool(port_info::socket_id(port_id));
                      auto success = utils::configure_port(
                          port_id, mempool, q_count.rx, q_count.tx);
                      if (!success) {
                          throw std::runtime_error(success.error());
                      }
                  });
}

template <typename ProcessType> eal_process<ProcessType>::eal_process()
{
    assert(rte_eal_process_type() == RTE_PROC_PRIMARY);

    /* Setup our ports; retrieve the set of port indexes */
    if constexpr (detail::has_port_setup_v<ProcessType>) {
        static_cast<ProcessType*>(this)->do_port_setup();
    }

    /* Generate queue configuration and memory pools */
    auto port_indexes = topology::get_ports();
    auto q_descriptors = topology::queue_distribute(port_indexes);
    log_idle_workers(q_descriptors);
    auto q_counts = queue::get_port_queue_counts(q_descriptors);

    pool_allocator::instance().init(port_indexes, q_counts);
    configure_all_ports(q_counts, pool_allocator::instance());

    if constexpr (detail::has_port_callbacks_v<ProcessType>) {
        static_cast<ProcessType*>(this)->do_port_callbacks();
    }
}

template <typename ProcessType> eal_process<ProcessType>::~eal_process()
{
    if constexpr (detail::has_cleanup_v<ProcessType>) {
        static_cast<ProcessType*>(this)->do_cleanup();
    }
    pool_allocator::instance().fini();

    /* XXX: causes core dump on exit due to module shutdown ordering issues */
    // rte_eal_cleanup();
}

template <typename ProcessType>
std::optional<port::generic_port>
eal_process<ProcessType>::get_port(uint16_t port_idx, std::string_view id) const
{
    return (physical_port(port_idx,
                          id,
                          pool_allocator::instance().get_mempool(
                              port_info::socket_id(port_idx))));
}

template <typename ProcessType>
void eal_process<ProcessType>::start_port(uint16_t port_idx)
{
    if (auto success = utils::start_port(port_idx); !success) {
        OP_LOG(OP_LOG_ERROR, "%s\n", success.error().c_str());
    } else {
        OP_LOG(OP_LOG_DEBUG, "Successfully started DPDK port %u\n", port_idx);
    }
}

template <typename ProcessType>
void eal_process<ProcessType>::stop_port(uint16_t port_idx)
{
    if (auto success = utils::stop_port(port_idx); !success) {
        OP_LOG(OP_LOG_ERROR, "%s\n", success.error().c_str());
    } else {
        OP_LOG(OP_LOG_DEBUG, "Successfully stopped DPDK port %u\n", port_idx);
    }
}

void live_port_process::do_port_callbacks()
{
    if (auto error = rte_eth_dev_callback_register(RTE_ETH_ALL,
                                                   RTE_ETH_EVENT_INTR_LSC,
                                                   log_link_status_change,
                                                   nullptr)) {
        OP_LOG(OP_LOG_WARNING,
               "Could not register link status change callback: %s\n",
               rte_strerror(std::abs(error)));
    }
}

void live_port_process::do_cleanup()
{
    rte_eth_dev_callback_unregister(
        RTE_ETH_ALL, RTE_ETH_EVENT_INTR_LSC, log_link_status_change, nullptr);
}

void test_port_process::do_port_setup()
{
    constexpr auto eth_prefix = "net_ring";
    constexpr auto ring_prefix = "test_ring";
    constexpr auto ring_flags = RING_F_SP_ENQ | RING_F_SC_DEQ;
    constexpr auto ring_slots = 256;
    constexpr auto socket_id = SOCKET_ID_ANY;

    const auto nb_test_portpairs = config::dpdk_test_portpairs();
    assert(nb_test_portpairs);

    /* Generate rings for our test ports to use */
    using ring_pair = std::pair<rte_ring*, rte_ring*>;
    auto ring_pairs = std::vector<ring_pair>{};
    unsigned idx = 0;
    std::generate_n(
        std::back_inserter(ring_pairs), nb_test_portpairs, [&]() -> ring_pair {
            auto name0 = ring_prefix + std::to_string(idx++);
            auto ring0 = rte_ring_create(
                name0.c_str(), ring_slots, socket_id, ring_flags);
            if (!ring0) {
                throw std::runtime_error("Failed to create test port ring "
                                         + name0);
            }

            auto name1 = ring_prefix + std::to_string(idx++);
            auto ring1 = rte_ring_create(
                name1.c_str(), ring_slots, socket_id, ring_flags);
            if (!ring1) {
                throw std::runtime_error("Failed to create test port ring "
                                         + name1);
            }

            return {ring0, ring1};
        });

    assert(ring_pairs.size() == static_cast<size_t>(nb_test_portpairs));

    /* Finally, create test port pairs */
    idx = 0;
    std::for_each(
        std::begin(ring_pairs), std::end(ring_pairs), [&](auto& pair) {
            auto name0 = eth_prefix + std::to_string(idx++);
            if (rte_eth_from_rings(name0.c_str(),
                                   std::addressof(pair.first),
                                   1,
                                   std::addressof(pair.second),
                                   1,
                                   socket_id)
                == -1) {
                throw std::runtime_error(
                    "Failed to create test port device: "
                    + std::string(rte_strerror(rte_errno)));
            }

            auto name1 = eth_prefix + std::to_string(idx++);
            if (rte_eth_from_rings(name1.c_str(),
                                   std::addressof(pair.second),
                                   1,
                                   std::addressof(pair.first),
                                   1,
                                   socket_id)
                == -1) {
                throw std::runtime_error(
                    "Failed to create test port device: "
                    + std::string(rte_strerror(rte_errno)));
            }
        });
}

/* Instantiate process flavors */
template class eal_process<live_port_process>;
template class eal_process<test_port_process>;

} // namespace openperf::packetio::dpdk::primary
