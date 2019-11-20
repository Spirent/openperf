#include "packetio/transmit_table.tcc"
#include "packetio/workers/dpdk/tx_source.h"

namespace openperf::packetio {

template class transmit_table<dpdk::tx_source>;

}
