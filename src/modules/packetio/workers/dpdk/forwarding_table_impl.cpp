#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/forwarding_table.tcc"
#include "packetio/generic_interface.hpp"
#include "packetio/generic_sink.hpp"

namespace openperf::packetio {

template <>
std::string get_interface_id(const interface::generic_interface& intf)
{
    return (intf.id());
}

template class forwarding_table<interface::generic_interface,
                                packet::generic_sink,
                                RTE_MAX_ETHPORTS>;

} // namespace openperf::packetio
