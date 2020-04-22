#include "packet/generator/traffic/protocol/mpls.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::mpls& mpls,
                    const libpacket::protocol::mpls&)
{
    set_mpls_bottom_of_stack(mpls, false);
}

} // namespace openperf::packet::generator::traffic::protocol
