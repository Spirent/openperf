#include "icp_tdigest.h"
#include "icp_tdigest.tcc"

template class icp::stats::icp_tdigest<double, uint64_t>;
template class icp::stats::icp_tdigest<uint16_t, uint64_t>;
