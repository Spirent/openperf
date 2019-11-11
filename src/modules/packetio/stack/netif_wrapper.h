#ifndef _ICP_PACKETIO_STACK_NETIF_WRAPPER_H_
#define _ICP_PACKETIO_STACK_NETIF_WRAPPER_H_

#include <any>
#include <string>

#include "net/net_types.h"
#include "packetio/generic_interface.h"

struct netif;

namespace icp::packetio {

class netif_wrapper
{
    const std::string m_id;
    const netif* m_netif;
    const interface::config_data m_config;

public:
    netif_wrapper(std::string_view id, const netif* ifp,
                  const interface::config_data& config);

    std::string id() const;
    std::string port_id() const;
    std::string mac_address() const;
    std::string ipv4_address() const;
    interface::config_data config() const;
    std::any data() const;
    interface::stats_data stats() const;
};

}

#endif /* _ICP_PACKETIO_STACK_NETIF_WRAPPER_H_ */
