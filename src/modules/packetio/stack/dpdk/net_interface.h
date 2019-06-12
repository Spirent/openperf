#ifndef _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_
#define _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_

#include <memory>

#include "lwip/netifapi.h"

#include "packetio/generic_driver.h"
#include "packetio/generic_interface.h"

struct pbuf;
struct rte_ring;
extern void rte_ring_free(struct rte_ring*);

namespace icp {
namespace packetio {
namespace dpdk {

class net_interface {
public:
    net_interface(std::string_view id, const interface::config_data& config,
                  driver::tx_burst tx);
    ~net_interface();

    static void *operator new(size_t);
    static void operator delete(void*);

    net_interface(const net_interface&) = delete;
    net_interface& operator= (const net_interface&) = delete;

    netif* data();

    std::string id() const;
    int port_id() const;
    interface::config_data config() const;

    unsigned max_gso_length() const;

    int attach_sink(pga::generic_sink& sink);
    void detach_sink(pga::generic_sink& sink);

    int attach_source(pga::generic_source& source);
    void detach_source(pga::generic_source& source);

    int handle_rx(struct pbuf*);
    int handle_tx(struct pbuf*);
    void handle_input();

private:
    struct rte_ring_deleter {
        void operator()(rte_ring *ring) {
            rte_ring_free(ring);
        }
    };

    /* XXX: Determine based on NIC speed */
    static constexpr unsigned recvq_size = 1024;

    const std::string m_id;
    const unsigned m_max_gso_length;
    const interface::config_data m_config;
    const driver::tx_burst m_transmit;

    netif m_netif;
    std::atomic_flag m_notify;
    std::unique_ptr<rte_ring, rte_ring_deleter> m_recvq;
};

}
}
}

#endif /* _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_ */
