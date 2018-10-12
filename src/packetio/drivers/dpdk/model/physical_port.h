#ifndef _ICP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_H_
#define _ICP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_H_

#include "packetio/generic_port.h"

struct rte_eth_conf;
struct rte_eth_dev_info;
struct rte_mempool;

namespace icp {
namespace packetio {
namespace dpdk {
namespace model {

class port_info;

class physical_port
{
public:
    physical_port(int id, rte_mempool *rx_pool);

    int id() const;

    static std::string kind();

    port::link_status link()   const;
    port::link_speed  speed()  const;
    port::link_duplex duplex() const;

    port::stats_data  stats()  const;

    port::config_data config() const;
    tl::expected<void, std::string> config(const port::config_data&);

    tl::expected<void, std::string> start();
    tl::expected<void, std::string> stop();

    tl::expected<void, std::string> low_level_config();

private:
    int m_id;
    rte_mempool *m_pool;

    tl::expected<void, std::string> apply_port_config(port_info& defaults,
                                                      rte_eth_dev_info &info,
                                                      rte_eth_conf& conf);
};

}
}
}
}
#endif /* _ICP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_H_ */
