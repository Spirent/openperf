#ifndef _OP_PACKET_GENERATOR_INTERFACE_SOURCE_HPP_
#define _OP_PACKET_GENERATOR_INTERFACE_SOURCE_HPP_

#include "packet/generator/learning.hpp"
#include "packetio/generic_interface.hpp"
#include "packet/generator/traffic/sequence.hpp"

namespace openperf::packet::generator {

class interface_source
{
public:
    explicit interface_source(core::event_loop& loop,
                              packetio::interface::generic_interface& intf);

    bool start_learning(traffic::sequence& sequence);
    void stop_learning();
    bool retry_learning(traffic::sequence& sequence);
    bool learning_resolved() const { return (m_learning_resolved); }
    void populate_source_addresses(traffic::sequence& sequence);

    std::string target_port() const;

    const learning_state_machine& learning() const { return (m_learning); }

private:
    learning_state_machine m_learning;
    packetio::interface::generic_interface m_interface;
    bool m_learning_resolved;
};
} // namespace openperf::packet::generator

#endif /* _OP_PACKET_GENERATOR_INTERFACE_SOURCE_HPP_ */