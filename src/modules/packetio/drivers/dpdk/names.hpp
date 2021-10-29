#ifndef _OP_PACKETIO_DRIVER_DPDK_NAMES_HPP_
#define _OP_PACKETIO_DRIVER_DPDK_NAMES_HPP_

#include <string>

namespace openperf::packetio::dpdk::driver_names {

inline constexpr std::string_view ring = "net_ring";
inline constexpr std::string_view virtio = "net_virtio";
inline constexpr std::string_view mlx5 = "mlx5_pci";

} // namespace openperf::packetio::dpdk::driver_names

#endif /* _OP_PACKETIO_DRIVER_DPDK_NAMES_HPP_ */
