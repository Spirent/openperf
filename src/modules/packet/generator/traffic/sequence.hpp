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
    /* Everything needed to generate a packet: */
    using view_type =
        std::tuple<unsigned,                             /* flow idx */
                   const uint8_t*,                       /* pointer to header */
                   packetio::packet::header_lengths,     /* header length */
                   packetio::packet::packet_type::flags, /* protocol flags */
                   std::optional<signature_config>,      /* signature? */
                   uint16_t>;                            /* packet length  */
    using iterator = view_iterator<sequence>;
    using const_iterator = const iterator;

    /* Named constructors to simplify instantiation. */
    static sequence round_robin_sequence(definition_container&& definitions);
    static sequence sequential_sequence(definition_container&& definitions);

    uint16_t max_packet_length() const;

    /* Retrieves the sum of packet lengths for a full sequence of packets. */
    size_t sum_packet_lengths() const;

    /*
     * Retrieves the sum of packet lengths from 0 to an arbitrary sequence
     * index.
     */
    size_t sum_packet_lengths(size_t pkt_idx) const;

    /* Same as above, but for specific flow indexes */
    size_t sum_flow_packet_lengths(unsigned flow_idx) const;
    size_t sum_flow_packet_lengths(unsigned flow_idx, size_t pkt_idx) const;
    /*
     * Indicates that at least one packet in the sequence contains a
     * signature configuration.
     */
    bool has_signature_config() const;

    /* Retrieve the stream if for the specified flow, if available. */
    std::optional<uint32_t> get_signature_stream_id(unsigned flow_idx) const;

    /* The number of unique headers in the sequence. */
    size_t flow_count() const;

    /* The number of occurences of the specified flow index in the sequence */
    size_t flow_packets(unsigned flow_idx) const;

    /*
     * The size of the sequence in terms of header/length pairs. Each header
     * is guaranteed to appear at least once in `size()` packets.
     */
    size_t size() const;

    /* Bulk retrieval of packet views from [start_idx, start_idx + count) */
    uint16_t unpack(size_t start_idx,
                    uint16_t count,
                    unsigned flow_indexes[],
                    const uint8_t* headers[],
                    packetio::packet::header_lengths header_lengths[],
                    packetio::packet::packet_type::flags header_flags[],
                    std::optional<signature_config> signature_configs[],
                    uint16_t pkt_lengths[]) const;

    view_type operator[](size_t idx) const;

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;

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
    std::vector<size_t> m_flow_offsets;
    size_t m_size;
};

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_ */
