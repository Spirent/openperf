#include "packetio/drivers/dpdk/port_stats.hpp"

#include <array>
#include <atomic>
#include <cassert>

#include "packetio/drivers/dpdk/dpdk.h"

/**
 * Custom stats are a bit of a hack, as they are useful for testing,
 * but don't fit into our existing code structure very well.  However,
 * take a hint from DPDK and use the fact that DPDK ports are
 * singletons, identified by an id, to make a custom stats
 * implementation that is easy and straight-forward.
 */

namespace openperf::packetio::dpdk::port_stats {

struct custom_port_stats
{
    std::atomic_uint_fast64_t tx_deferred;
};

static auto custom_stats = std::array<custom_port_stats, RTE_MAX_ETHPORTS>{};

uint64_t tx_deferred(uint16_t port_id)
{
    assert(port_id < RTE_MAX_ETHPORTS);

    return (custom_stats[port_id].tx_deferred.load(std::memory_order_relaxed));
}

void tx_deferred_add(uint16_t port_id, uint64_t inc)
{
    assert(port_id < RTE_MAX_ETHPORTS);

    custom_stats[port_id].tx_deferred.fetch_add(inc, std::memory_order_relaxed);
}

} // namespace openperf::packetio::dpdk::port_stats
