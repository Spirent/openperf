#ifndef _OP_PACKETIO_STACK_NETIF_WRAPPER_HPP_
#define _OP_PACKETIO_STACK_NETIF_WRAPPER_HPP_

#include <any>
#include <string>

#include "net/net_types.hpp"
#include "packetio/generic_interface.hpp"

struct netif;

namespace openperf::packetio {

class netif_wrapper
{
    const std::string m_id;
    const netif* m_netif;
    const interface::config_data m_config;

public:
    netif_wrapper(std::string_view id,
                  const netif* ifp,
                  const interface::config_data& config);

    std::string id() const;
    std::string port_id() const;
    std::string mac_address() const;
    std::string ipv4_address() const;
    std::string ipv6_address() const;
    std::string ipv6_linklocal_address() const;
    interface::config_data config() const;
    std::any data() const;
    interface::stats_data stats() const;
};

} // namespace openperf::packetio

#endif /* _OP_PACKETIO_STACK_NETIF_WRAPPER_HPP_ */
