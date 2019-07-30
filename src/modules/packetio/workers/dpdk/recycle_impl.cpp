#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/recycle.tcc"

template class icp::packetio::recycle::depot<RTE_MAX_LCORE>;
template class icp::packetio::recycle::guard<RTE_MAX_LCORE>;
