#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/stack/dpdk/net_interface.hpp"
#include "packetio/forwarding_table.tcc"
#include "packetio/generic_sink.hpp"

struct netif;

namespace openperf::packetio {

/*
 * Provide a template specialization for the forwarding table so that it
 * can retrieve the opaque string id from an interface.
 */
template <> std::string get_interface_id(netif* ifp)
{
    return (dpdk::to_interface(ifp).id());
}

template class forwarding_table<netif, packet::generic_sink, RTE_MAX_ETHPORTS>;

} // namespace openperf::packetio
