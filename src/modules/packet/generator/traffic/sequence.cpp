#include "packet/generator/traffic/sequence.hpp"

namespace openperf::packet::generator::traffic {

template <typename Strategy>
sequence_view<Strategy>::sequence_view(definition_container&& definitions)
{
    m_templates.reserve(definitions.size());
    m_idx_offsets.reserve(definitions.size());

    std::transform(std::begin(definitions),
                   std::end(definitions),
                   std::back_inserter(m_templates),
                   [](auto&& item) { return (std::move(item.first)); });

    auto offset = 0U;
    std::transform(std::begin(m_templates),
                   std::end(m_templates),
                   std::back_inserter(m_idx_offsets),
                   [&](const auto& item) {
                       auto tmp = offset;
                       offset += item.size();
                       return (tmp);
                   });
}

template <typename Strategy>
uint16_t sequence_view<Strategy>::max_packet_length() const
{
    return (std::accumulate(std::begin(m_templates),
                            std::end(m_templates),
                            0U,
                            [](uint16_t lhs, const auto& rhs) {
                                return (std::max(lhs, rhs.max_packet_length()));
                            }));
}

template <typename Strategy>
size_t sequence_view<Strategy>::sum_packet_lengths() const
{
    return (std::accumulate(
        std::begin(*this),
        std::end(*this),
        0UL,
        [](size_t lhs, const auto& rhs) { return (lhs + std::get<2>(rhs)); }));
}

template <typename Strategy>
size_t sequence_view<Strategy>::sum_packet_lengths(size_t pkt_idx) const
{
    const auto seq_sum = sum_packet_lengths();
    auto q = lldiv(pkt_idx, size());
    return (q.quot * seq_sum
            + std::accumulate(std::begin(*this),
                              std::next(std::begin(*this), q.rem),
                              0UL,
                              [](size_t lhs, const auto& rhs) {
                                  return (lhs + std::get<2>(rhs));
                              }));
}

template <typename Strategy> size_t sequence_view<Strategy>::flow_count() const
{
    return (std::accumulate(
        std::begin(m_templates),
        std::end(m_templates),
        0U,
        [](size_t lhs, const auto& rhs) { return (lhs + rhs.size()); }));
}

template <typename Strategy> size_t sequence_view<Strategy>::size() const
{
    return (static_cast<const Strategy&>(*this).get_size());
}

template <typename Strategy>
typename sequence_view<Strategy>::view_type
    sequence_view<Strategy>::operator[](size_t idx) const
{
    auto&& [tmp_idx, pkt_idx] =
        static_cast<const Strategy&>(*this).get_indexes(idx);
    pkt_idx %= m_templates[tmp_idx].size();
    auto&& [hdr_desc, pkt_len] = m_templates[tmp_idx][pkt_idx];
    return {m_idx_offsets[tmp_idx] + pkt_idx, hdr_desc, pkt_len};
}

template <typename Strategy>
bool sequence_view<Strategy>::operator==(const sequence_view& other) const
{
    return (static_cast<const Strategy&>(*this)
                == static_cast<const Strategy&>(other)
            && m_templates == other.m_templates);
}

template <typename Strategy>
typename sequence_view<Strategy>::iterator sequence_view<Strategy>::begin()
{
    return (iterator(*this));
}

template <typename Strategy>
typename sequence_view<Strategy>::const_iterator
sequence_view<Strategy>::begin() const
{
    return (iterator(*this));
}

template <typename Strategy>
typename sequence_view<Strategy>::iterator sequence_view<Strategy>::end()
{
    return (iterator(*this, size()));
}

template <typename Strategy>
typename sequence_view<Strategy>::const_iterator
sequence_view<Strategy>::end() const
{
    return (iterator(*this, size()));
}

template <typename Strategy>
const typename sequence_view<Strategy>::packet_template_container&
sequence_view<Strategy>::templates() const
{
    return (m_templates);
}

/* Instantiate template based sequence views */
template class sequence_view<round_robin_sequence>;
template class sequence_view<sequential_sequence>;

round_robin_sequence::round_robin_sequence(definition_container&& definitions)
    : sequence_view(std::forward<definition_container>(definitions))
{
    m_weights.reserve(definitions.size());
    m_weights_partial_sums.reserve(definitions.size());

    std::transform(std::begin(definitions),
                   std::end(definitions),
                   std::back_inserter(m_weights),
                   [](const auto& item) { return (item.second); });

    std::partial_sum(std::begin(m_weights),
                     std::end(m_weights),
                     std::back_inserter(m_weights_partial_sums));
}

size_t round_robin_sequence::get_size() const
{
    return (std::inner_product(
        std::begin(templates()),
        std::end(templates()),
        std::begin(m_weights),
        0U,
        std::plus<size_t>{},
        [](const auto& lhs, size_t rhs) { return (lhs.size() * rhs); }));
}

std::pair<size_t, size_t> round_robin_sequence::get_indexes(size_t idx) const
{
    auto q = ldiv(idx, m_weights_partial_sums.back());

    auto tmp_idx =
        std::distance(std::begin(m_weights_partial_sums),
                      std::upper_bound(std::begin(m_weights_partial_sums),
                                       std::end(m_weights_partial_sums),
                                       q.rem));

    auto offset = q.quot * m_weights_partial_sums.back()
                  + (tmp_idx == 0 ? 0 : m_weights_partial_sums[tmp_idx - 1])
                  - q.quot * m_weights[tmp_idx];

    assert(static_cast<size_t>(offset) <= idx);

    return {tmp_idx, idx - offset};
}

bool round_robin_sequence::operator==(const round_robin_sequence& other) const
{
    return (m_weights == other.m_weights);
}

sequential_sequence::sequential_sequence(definition_container&& definitions)
    : sequence_view(std::forward<definition_container>(definitions))
{
    m_lengths.reserve(definitions.size());
    m_lengths_partial_sums.reserve(definitions.size());

    std::transform(std::begin(definitions),
                   std::end(definitions),
                   std::begin(templates()),
                   std::back_inserter(m_lengths),
                   [](const auto& item, const auto& t) {
                       return (item.second * t.size());
                   });

    std::partial_sum(std::begin(m_lengths),
                     std::end(m_lengths),
                     std::back_inserter(m_lengths_partial_sums));
}

size_t sequential_sequence::get_size() const
{
    return (m_lengths_partial_sums.back());
}

std::pair<size_t, size_t> sequential_sequence::get_indexes(size_t idx) const
{
    auto q = ldiv(idx, m_lengths_partial_sums.back());

    auto tmp_idx =
        std::distance(std::begin(m_lengths_partial_sums),
                      std::upper_bound(std::begin(m_lengths_partial_sums),
                                       std::end(m_lengths_partial_sums),
                                       q.rem));

    auto offset = q.quot * m_lengths_partial_sums.back()
                  + (tmp_idx == 0 ? 0 : m_lengths_partial_sums[tmp_idx - 1])
                  - q.quot * m_lengths[tmp_idx];

    assert(static_cast<size_t>(offset) <= idx);

    return {tmp_idx, idx - offset};
}

bool sequential_sequence::operator==(const sequential_sequence& other) const
{
    return (m_lengths == other.m_lengths);
}

} // namespace openperf::packet::generator::traffic
