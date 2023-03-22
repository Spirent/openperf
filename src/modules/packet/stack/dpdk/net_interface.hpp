#ifndef _OP_PACKET_STACK_DPDK_NET_INTERFACE_HPP_
#define _OP_PACKET_STACK_DPDK_NET_INTERFACE_HPP_

#include <memory>
#include <variant>

#include "lwip/netif.h"

#include "packet/stack/dpdk/packet_filter.hpp"
#include "packetio/generic_interface.hpp"
#include "packetio/generic_workers.hpp"

struct pbuf;
struct rte_mbuf;

namespace openperf::packet::stack::dpdk {

struct netif_ext
{
    // lwip doesn't support IPv6 prefix length or default gateway, so these need
    // to be stored outside of the netif
    std::array<uint8_t, LWIP_IPV6_NUM_ADDRESSES> ip6_address_prefix_len;
    std::optional<ip6_addr_t> ip6_gateway;
};

class net_interface
{
public:
    net_interface(std::string_view id,
                  int port_index,
                  const packetio::interface::config_data& config,
                  packetio::workers::transmit_function tx);
    ~net_interface();

    static void* operator new(size_t);
    static void operator delete(void*);

    net_interface(const net_interface&) = delete;
    net_interface& operator=(const net_interface&) = delete;

    const netif* data() const;

    std::string id() const;
    std::string port_id() const;
    int port_index() const;
    packetio::interface::config_data config() const;

    unsigned max_gso_length() const;

    void handle_link_state_change(bool link_up);

    bool accept(const rte_mbuf* mbuf) const;
    bool is_up() const;
    void up();
    void down();

    err_t handle_tx(struct pbuf*);

    friend netif_ext& get_netif_ext(netif*);

private:
    void configure();
    void unconfigure();

    const std::string m_id;
    const int m_port_index; /* DPDK port index, that is */
    const unsigned m_max_gso_length;
    const packetio::interface::config_data m_config;
    const packetio::workers::transmit_function m_transmit;
    const packet_filter m_rx_filter;
    const packet_filter m_tx_filter;

    netif m_netif;
    netif_ext m_netif_ext;
};

const net_interface& to_interface(netif*);

netif_ext& get_netif_ext(netif*);

} // namespace openperf::packet::stack::dpdk

#endif /* _OP_PACKET_STACK_DPDK_NET_INTERFACE_HPP_ */
