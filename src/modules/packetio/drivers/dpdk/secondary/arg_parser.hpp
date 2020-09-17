#ifndef _OP_PACKETIO_DPDK_SECONDARY_ARG_PARSER_HPP_
#define _OP_PACKETIO_DPDK_SECONDARY_ARG_PARSER_HPP_

#include "packetio/drivers/dpdk/arg_parser.hpp"

extern const char op_packetio_dpdk_tx_mempools[];
extern const char op_packetio_dpdk_tx_queues[];

namespace openperf::packetio::dpdk::config {

std::map<uint16_t, std::vector<uint16_t>> tx_port_queues();

} // namespace openperf::packetio::dpdk::config

#endif /* _OP_PACKETIO_DPDK_SECONDARY_ARG_PARSER_HPP_ */
