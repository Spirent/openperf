#include "packetio/internal_worker.h"
#include "packetio/drivers/dpdk/dpdk.h"

namespace icp::packetio::internal::worker {

unsigned get_id()
{
    return (rte_lcore_id());
}

unsigned get_numa_node()
{
    return (rte_socket_id());
}

}
