#ifndef _OP_PACKETIO_DPDK_PRIMARY_ARG_PARSER_HPP_
#define _OP_PACKETIO_DPDK_PRIMARY_ARG_PARSER_HPP_

#include "packetio/drivers/dpdk/arg_parser.hpp"

extern const char op_packetio_dpdk_no_lro[];
extern const char op_packetio_dpdk_no_rx_irqs[];
extern const char op_packetio_dpdk_test_mode[];
extern const char op_packetio_dpdk_test_portpairs[];

namespace openperf::packetio::dpdk::config {

int dpdk_test_portpairs(); /**< Number of eth ring devs */
bool dpdk_test_mode();     /**< test mode enable/disable */

} // namespace openperf::packetio::dpdk::config

#endif /* _OP_PACKETIO_DPDK_PRIMARY_ARG_PARSER_HPP_ */
