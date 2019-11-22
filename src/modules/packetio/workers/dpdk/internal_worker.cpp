#include "packetio/internal_worker.hpp"
#include "packetio/drivers/dpdk/dpdk.h"

namespace openperf::packetio::internal::worker {

unsigned get_id() { return (rte_lcore_id()); }

unsigned get_numa_node() { return (rte_socket_id()); }

} // namespace openperf::packetio::internal::worker
