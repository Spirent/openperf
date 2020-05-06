#ifndef _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_

#include "packet/generator/traffic/length_template.hpp"
#include "packet/generator/traffic/packet_template.hpp"
#include "packet/generator/traffic/signature_config.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packet::generator::traffic {

/* packet template, length_template, weight, signature config */
using definition = std::tuple<packet_template,
                              length_template,
                              unsigned,
                              std::optional<signature_config>>;
using definition_container = std::vector<definition>;

class sequence
{
public:
    static sequence round_robin_sequence(definition_container&& definitions);
    static sequence sequential_sequence(definition_container&& definitions);

    uint16_t max_packet_length() const;

    size_t sum_packet_lengths() const;
    size_t sum_packet_lengths(size_t pkt_idx) const;

    bool has_signature_config() const;

    size_t flow_count() const;

    size_t size() const;

    uint16_t unpack(size_t start_idx,
                    uint16_t count,
                    unsigned flow_indexes[],
                    const uint8_t* headers[],
                    packetio::packet::header_lengths header_lengths[],
                    packetio::packet::packet_type::flags header_flags[],
                    std::optional<signature_config> signature_configs[],
                    uint16_t pkt_lengths[]) const;

protected:
    enum class order_type { round_robin, sequential };
    sequence(definition_container&& definitions, order_type order);

private:
    using soa_definition_container =
        utils::soa_container<std::vector, definition>;
    soa_definition_container m_definitions;

    using index_pair = std::pair<uint16_t, uint16_t>;
    using index_container = std::vector<index_pair>;

    index_container m_packet_indexes;
    index_container m_length_indexes;
    std::vector<size_t> m_flow_offsets;
};

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_ */
