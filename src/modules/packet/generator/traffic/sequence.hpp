#ifndef _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_

#include "packet/generator/traffic/packet_template.hpp"

namespace openperf::packet::generator::traffic {

using definition = std::pair<packet_template, unsigned>;
using definition_container = std::vector<definition>;

template <typename Strategy> class sequence_view
{
public:
    /* flow idx, header descriptor, pkt length */
    using view_type =
        std::tuple<size_t, packet_template::header_descriptor, uint16_t>;
    using packet_template_container = std::vector<packet_template>;
    using iterator = view_iterator<sequence_view>;
    using const_iterator = const iterator;

    sequence_view() = default;
    sequence_view(definition_container&& definitions);

    virtual ~sequence_view() = default;

    uint16_t max_packet_length() const;

    /**
     * The sum of packet lengths in a sequence iteration.  Dividing this
     * number by the size() property gives your the average packet length.
     */
    size_t sum_packet_lengths() const;

    /**
     * Calculate the sum of packet lengths for the number of packets up to
     * pkt_idx. Note: pkt_idx may be larger than size.
     */
    size_t sum_packet_lengths(size_t pkt_idx) const;

    /* The number of unique flows */
    size_t flow_count() const;

    /* The number of packets in a sequence iteration */
    size_t size() const;

    view_type operator[](size_t idx) const;

    bool operator==(const sequence_view& other) const;

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;

protected:
    const packet_template_container& templates() const;

private:
    packet_template_container m_templates;
    std::vector<unsigned> m_idx_offsets;
};

class round_robin_sequence final : public sequence_view<round_robin_sequence>
{
    std::vector<unsigned> m_weights;
    std::vector<unsigned> m_weights_partial_sums;

public:
    round_robin_sequence() = default;
    round_robin_sequence(definition_container&&);

    size_t get_size() const;
    std::pair<size_t, size_t> get_indexes(size_t) const;
    bool operator==(const round_robin_sequence&) const;
};

struct sequential_sequence final : public sequence_view<sequential_sequence>
{
    std::vector<unsigned> m_lengths;
    std::vector<unsigned> m_lengths_partial_sums;

public:
    sequential_sequence() = default;
    sequential_sequence(definition_container&& config);

    size_t get_size() const;
    std::pair<size_t, size_t> get_indexes(size_t) const;
    bool operator==(const sequential_sequence&) const;
};

using sequence = std::variant<round_robin_sequence, sequential_sequence>;

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_SEQUENCE_HPP_ */
