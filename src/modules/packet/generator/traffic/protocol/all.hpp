#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ALL_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ALL_HPP_

#include "core/op_log.h"

#include "packet/generator/traffic/protocol/ethernet.hpp"
#include "packet/generator/traffic/protocol/vlan.hpp"
#include "packet/generator/traffic/protocol/mpls.hpp"
#include "packet/generator/traffic/protocol/ip.hpp"
#include "packet/generator/traffic/protocol/protocol.hpp"

namespace openperf::packet::generator::traffic::protocol {

template <typename Previous, typename Next>
void update_context(Previous&, const Next&)
{
    /* dummy next header handler */
    OP_LOG(
        OP_LOG_WARNING, "No explicit definition for %s\n", __PRETTY_FUNCTION__);
}

template <typename Protocol> void update_length(Protocol&, uint16_t)
{
    /* dummy length handler */
}

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ALL_HPP_ */
