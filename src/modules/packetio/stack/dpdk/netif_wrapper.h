#ifndef _ICP_PACKETIO_STACK_NETIF_WRAPPER_H_
#define _ICP_PACKETIO_STACK_NETIF_WRAPPER_H_

#include "packetio/generic_interface.h"

struct netif;

namespace icp {
namespace packetio {
namespace dpdk {

class netif_wrapper
{
    const std::string m_id;
    const netif* m_netif;
    const interface::config_data m_config;

public:
    netif_wrapper(const std::string& id, const netif* ifp,
                  const interface::config_data& config);

    std::string id() const;
    int port_id() const;
    std::string mac_address() const;
    std::string ipv4_address() const;
    interface::stats_data stats() const;
    interface::config_data config() const;
};

}
}
}
#endif /* _ICP_PACKETIO_STACK_NETIF_WRAPPER_H_ */
