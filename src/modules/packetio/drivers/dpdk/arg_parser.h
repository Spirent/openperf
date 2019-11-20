#ifndef _OP_PACKETIO_DPDK_ARG_PARSER_H_
#define _OP_PACKETIO_DPDK_ARG_PARSER_H_

#include <string>
#include <vector>
#include <unordered_map>

namespace openperf::packetio::dpdk::config {

int dpdk_test_portpairs();                          /**< Number of eth ring devs */
bool dpdk_test_mode();                              /**< test mode enable/disable */
std::vector<std::string> dpdk_args();               /**< Retrieve a copy of args for use */
std::unordered_map<int, std::string> dpdk_id_map(); /**< Retrieve a copy of port idx->id map */

} /* namespace openperf::packetio::dpdk::config */


#endif /* _OP_PACKETIO_DPDK_ARG_PARSER_H_ */
