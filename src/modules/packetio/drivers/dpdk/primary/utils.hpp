#ifndef _OP_PACKETIO_DPDK_PRIMARY_UTILS_HPP_
#define _OP_PACKETIO_DPDK_PRIMARY_UTILS_HPP_

#include "packetio/generic_port.hpp"

struct rte_mempool;

namespace openperf::packetio::dpdk::primary::utils {

tl::expected<void, std::string> start_port(uint16_t port_id);

tl::expected<void, std::string> stop_port(uint16_t port_id);

tl::expected<void, std::string> configure_port(uint16_t port_idx,
                                               rte_mempool* const mempool,
                                               uint16_t nb_rxqs,
                                               uint16_t nb_txqs);

tl::expected<void, std::string>
reconfigure_port(uint16_t port_idx,
                 rte_mempool* const mempool,
                 const port::dpdk_config& config);

} // namespace openperf::packetio::dpdk::primary::utils

#endif /* _OP_PACKETIO_DPDK_PRIMARY_UTILS_HPP_ */
