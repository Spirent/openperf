#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/recycle.tcc"

template class openperf::packetio::recycle::depot<RTE_MAX_LCORE>;
template class openperf::packetio::recycle::guard<RTE_MAX_LCORE>;
