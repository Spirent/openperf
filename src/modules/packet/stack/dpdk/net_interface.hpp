#ifndef _OP_PACKET_STACK_DPDK_NET_INTERFACE_HPP_
#define _OP_PACKET_STACK_DPDK_NET_INTERFACE_HPP_

#include <memory>
#include <variant>

#include "lwip/netif.h"

#include "packetio/generic_interface.hpp"
#include "packetio/generic_workers.hpp"

struct pbuf;

namespace openperf::packet::stack::dpdk {

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

    bool is_up() const;
    void up();
    void down();

    err_t handle_tx(struct pbuf*);

private:
    void configure();
    void unconfigure();

    const std::string m_id;
    const int m_port_index; /* DPDK port index, that is */
    const unsigned m_max_gso_length;
    const packetio::interface::config_data m_config;
    const packetio::workers::transmit_function m_transmit;

    netif m_netif;
};

const net_interface& to_interface(netif*);

} // namespace openperf::packet::stack::dpdk

#endif /* _OP_PACKET_STACK_DPDK_NET_INTERFACE_HPP_ */
