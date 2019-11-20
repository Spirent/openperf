#include "lwip/netifapi.h"
#include "packetio/stack/dpdk/net_interface.hpp"

using namespace openperf::packetio::dpdk;

uint8_t netif_id_match(std::string_view id_str)
{
    for (auto i = 1; i <= UINT8_MAX; i++) {
        auto n = netif_get_by_index(i);
        if (n != nullptr) {
            auto *ifp =
                reinterpret_cast<openperf::packetio::dpdk::net_interface*>(n->state);
            if (ifp->id() == id_str) {
                return netif_get_index(n);
            }
        }
    }
    return (NETIF_NO_INDEX);
}
