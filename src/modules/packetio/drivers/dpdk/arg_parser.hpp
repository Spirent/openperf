#ifndef _OP_PACKETIO_DPDK_ARG_PARSER_HPP_
#define _OP_PACKETIO_DPDK_ARG_PARSER_HPP_

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "packetio/drivers/dpdk/model/core_mask.hpp"

namespace openperf::packetio::dpdk::config {

int dpdk_test_portpairs();            /**< Number of eth ring devs */
bool dpdk_test_mode();                /**< test mode enable/disable */
std::vector<std::string> dpdk_args(); /**< Retrieve a copy of args for use */
std::unordered_map<int, std::string>
dpdk_id_map(); /**< Retrieve a copy of port idx->id map */

std::optional<model::core_mask> misc_core_mask();
std::optional<model::core_mask> rx_core_mask();
std::optional<model::core_mask> tx_core_mask();

} /* namespace openperf::packetio::dpdk::config */

#endif /* _OP_PACKETIO_DPDK_ARG_PARSER_HPP_ */
