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

    static void *operator new(size_t);
    static void operator delete(void*);

    net_interface(const net_interface&) = delete;
    net_interface& operator= (const net_interface&) = delete;

    netif* data();

    int id() const;
    int port_id() const;
    interface::config_data config() const;

private:
    const interface::config_data m_config;
    const int m_id;
    alignas(64) netif m_netif;  /**< This member is for another thread to use */
};

}
}
}

#endif /* _ICP_PACKETIO_STACK_DPDK_NET_INTERFACE_H_ */
