#ifndef _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_
#define _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_

#include <memory>

#include "lwip/netifapi.h"

#include "packetio/generic_interface.h"

namespace icp {
namespace packetio {
namespace dpdk {


class net_interface {
public:
    net_interface(int id, const interface::config_data& config);
    ~net_interface();

    int id() const;
    int port_id() const;
    std::string mac_address() const;
    std::string ipv4_address() const;
    interface::stats_data stats() const;
    interface::config_data config() const;

private:
    interface::config_data m_config;
    netif m_netif;
    int m_id;
};

}
}
}

#endif /* _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_ */
