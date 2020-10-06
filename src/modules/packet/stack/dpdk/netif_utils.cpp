#include "lwip/netifapi.h"

#include "packet/stack/lwip/netif_utils.hpp"
#include "packet/stack/dpdk/net_interface.hpp"
#include "packet/stack/dpdk/tcpip_input.hpp"

namespace openperf::packet::stack {

uint8_t netif_id_match(std::string_view id_str)
{
    for (auto i = 1; i <= UINT8_MAX; i++) {
        auto n = netif_get_by_index(i);
        if (n != nullptr) {
            auto* ifp =
                reinterpret_cast<openperf::packet::stack::dpdk::net_interface*>(
                    n->state);
            if (ifp->id() == id_str) { return netif_get_index(n); }
        }
    }
    return (NETIF_NO_INDEX);
}

err_t netif_inject(struct netif* ifp, void* packet)
{
    return (dpdk::tcpip_input::instance().inject(
        ifp, reinterpret_cast<rte_mbuf*>(packet)));
}

} // namespace openperf::packet::stack
