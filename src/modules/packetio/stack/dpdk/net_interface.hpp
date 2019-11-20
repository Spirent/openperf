#ifndef _OP_PACKETIO_STACK_DPDK_NET_INTERFACE_HPP_
#define _OP_PACKETIO_STACK_DPDK_NET_INTERFACE_HPP_

#include <memory>
#include <variant>

#include "lwip/netif.h"

#include "packetio/generic_driver.hpp"
#include "packetio/generic_interface.hpp"
#include "packetio/generic_workers.hpp"
#include "packetio/stack/dpdk/net_interface_rx.hpp"

struct pbuf;

namespace openperf {
namespace packetio {
namespace dpdk {

class net_interface {
public:
    net_interface(std::string_view id, int port_index,
                  const interface::config_data& config,
                  driver::tx_burst tx);
    ~net_interface();

    static void *operator new(size_t);
    static void operator delete(void*);

    net_interface(const net_interface&) = delete;
    net_interface& operator= (const net_interface&) = delete;

    const netif* data() const;

    std::string id() const;
    std::string port_id() const;
    int port_index() const;
    interface::config_data config() const;

    unsigned max_gso_length() const;

    void handle_link_state_change(bool link_up);

    err_t handle_rx(struct pbuf*);
    err_t handle_rx_notify();

    err_t handle_tx(struct pbuf*);

    using rx_strategy = std::variant<netif_rx_strategy::direct,
                                     netif_rx_strategy::queueing>;
private:
    const std::string m_id;
    const int m_port_index; /* DPDK port index, that is */
    const unsigned m_max_gso_length;
    const interface::config_data m_config;
    const driver::tx_burst m_transmit;

    rx_strategy m_receive;

    netif m_netif;
};

const net_interface& to_interface(netif*);

}
}
}

#endif /* _OP_PACKETIO_STACK_DPDK_NET_INTERFACE_HPP_ */
