#include <net/if.h>

#include "lwip/netifapi.h"

#include "packet/stack/lwip/netif_utils.hpp"
#include "packet/stack/dpdk/net_interface.hpp"
#include "packet/stack/dpdk/tcpip_input.hpp"

namespace openperf::packet::stack {

const struct netif* netif_get_by_name(std::string_view id_str)
{
    netif* cursor = nullptr;
    NETIF_FOREACH (cursor) {
        auto* ifp =
            reinterpret_cast<openperf::packet::stack::dpdk::net_interface*>(
                cursor->state);
        if (ifp->id().compare(id_str) == 0) { return (cursor); }
    }
    return (nullptr);
}

uint8_t netif_index_by_name(std::string_view id_str)
{
    if (auto* netif = netif_get_by_name(id_str)) {
        return (netif_get_index(netif));
    }
    return (NETIF_NO_INDEX);
}

std::string interface_id_by_netif(const struct netif* n)
{
    auto* ifp = reinterpret_cast<openperf::packet::stack::dpdk::net_interface*>(
        n->state);
    return (ifp->id());
}

err_t netif_inject(struct netif* ifp, void* packet)
{
    return (dpdk::tcpip_input::instance().inject(
        ifp, reinterpret_cast<rte_mbuf*>(packet)));
}

} // namespace openperf::packet::stack
