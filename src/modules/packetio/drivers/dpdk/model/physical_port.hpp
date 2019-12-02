#ifndef _OP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_HPP_
#define _OP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_HPP_

#include "net/net_types.hpp"
#include "packetio/generic_port.hpp"

struct rte_eth_conf;
struct rte_eth_dev_info;
struct rte_mempool;

namespace openperf {
namespace packetio {
namespace dpdk {
namespace model {

class port_info;

class physical_port
{
public:
    physical_port(int idx, std::string_view id, rte_mempool* rx_pool = nullptr);

    std::string id() const;

    static std::string kind();

    port::link_status link() const;
    port::link_speed speed() const;
    port::link_duplex duplex() const;

    port::stats_data stats() const;

    port::config_data config() const;
    tl::expected<void, std::string> config(const port::config_data&);

    tl::expected<void, std::string> start();
    tl::expected<void, std::string> stop();

    tl::expected<void, std::string> low_level_config(uint16_t nb_rxqs,
                                                     uint16_t nb_txqs);

private:
    const int m_idx;        /* Global DPDK port index. */
    const std::string m_id; /* User-defined ID for REST API interactions. */
    const rte_mempool* m_pool;

    tl::expected<void, std::string> apply_port_config(port_info&,
                                                      rte_eth_conf&,
                                                      uint16_t nb_rxqs = 0,
                                                      uint16_t nb_txqs = 0);
};

} // namespace model
} // namespace dpdk
} // namespace packetio
} // namespace openperf
#endif /* _OP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_HPP_ */
