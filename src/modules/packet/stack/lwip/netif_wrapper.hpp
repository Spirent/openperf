#ifndef _OP_PACKET_STACK_NETIF_WRAPPER_HPP_
#define _OP_PACKET_STACK_NETIF_WRAPPER_HPP_

#include <any>
#include <string>

#include "packetio/generic_interface.hpp"

struct netif;

namespace openperf::packet::stack {

class netif_wrapper
{
    const std::string m_id;
    const netif* m_netif;
    const packetio::interface::config_data m_config;

public:
    netif_wrapper(std::string_view id,
                  const netif* ifp,
                  const packetio::interface::config_data& config);

    std::string id() const;
    std::string port_id() const;
    std::string mac_address() const;
    packetio::interface::dhcp_client_state dhcp_state() const;
    std::optional<std::string> ipv4_address() const;
    std::optional<std::string> ipv4_gateway() const;
    std::optional<uint8_t> ipv4_prefix_length() const;
    std::optional<std::string> ipv6_address() const;
    std::optional<std::string> ipv6_linklocal_address() const;
    packetio::interface::config_data config() const;
    std::any data() const;
    packetio::interface::stats_data stats() const;
    int input_packet(void* packet) const;
};

}

#endif /* _OP_PACKET_STACK_NETIF_WRAPPER_HPP_ */
