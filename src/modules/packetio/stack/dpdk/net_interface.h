#ifndef _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_
#define _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_

#include <memory>
#include <variant>

#include "lwip/netif.h"

#include "packetio/generic_driver.h"
#include "packetio/generic_interface.h"
#include "packetio/stack/dpdk/net_interface_rx.h"

struct pbuf;

namespace icp {
namespace packetio {
namespace dpdk {

class net_interface {
public:
    net_interface(std::string_view id, const interface::config_data& config,
                  driver::tx_burst tx, int port_index);
    ~net_interface();

    static void *operator new(size_t);
    static void operator delete(void*);

    net_interface(const net_interface&) = delete;
    net_interface& operator= (const net_interface&) = delete;

    struct netif* data();

    std::string id() const;
    std::string port_id() const;
    int port_index() const;
    interface::config_data config() const;

    unsigned max_gso_length() const;

    err_t handle_tx(struct pbuf*);

    err_t handle_rx(struct pbuf*);
    err_t handle_rx_notify();

private:
    const std::string m_id;
    const int m_port_index; /* DPDK port index, that is */
    const unsigned m_max_gso_length;
    const interface::config_data m_config;
    const driver::tx_burst m_transmit;

    typedef std::variant<netif_rx_strategy::direct,
                         netif_rx_strategy::queueing> rx_strategy;
    rx_strategy m_receive;

    netif m_netif;
};

}
}
}

#endif /* _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_ */
