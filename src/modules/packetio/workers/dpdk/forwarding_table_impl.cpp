#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/forwarding_table.tcc"
#include "packetio/generic_sink.h"

struct netif;

namespace icp::packetio {

template class forwarding_table<netif,
                                packets::generic_sink,
                                RTE_MAX_ETHPORTS>;

}
