#include "packetio/drivers/dpdk/dpdk.h"
#include "utils/recycle.tcc"

template class openperf::utils::recycle::depot<RTE_MAX_LCORE>;
template class openperf::utils::recycle::guard<RTE_MAX_LCORE>;
