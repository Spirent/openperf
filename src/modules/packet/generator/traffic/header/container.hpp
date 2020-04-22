#ifndef _OP_PACKET_GENERATOR_TRAFFIC_HEADER_CONTAINER_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_HEADER_CONTAINER_HPP_

#include <algorithm>
#include <numeric>
#include <vector>

#include "packet/generator/traffic/view_iterator.hpp"

namespace openperf::packet::generator::traffic::header {

/**
 * An ordered collection of headers.
 * Contained values are extracted via a view, e.g. a ptr and length.
 */

class container
{
    std::vector<uint8_t> m_data;
    std::vector<uint16_t> m_lengths;

public:
    using value_type = std::pair<const uint8_t*, uint16_t>;
    using view_type = value_type;
    using iterator = view_iterator<container>;
    using const_iterator = const iterator;

    void push_back(const value_type& value)
    {
        std::copy_n(value.first, value.second, std::back_inserter(m_data));
        m_lengths.push_back(value.second);
    }

    void push_back(value_type&& value) { push_back(value); }

    template <typename Header, typename Length>
    void emplace_back(const Header* ptr, Length length)
    {
        m_data.reserve(m_data.size() + length);
        std::copy_n(reinterpret_cast<const uint8_t*>(ptr),
                    length,
                    std::back_inserter(m_data));
        m_lengths.push_back(length);
    }

    bool empty() const { return (m_lengths.empty()); }

    size_t size() const { return (m_lengths.size()); }

    view_type operator[](size_t idx) const
    {
        auto offset =
            std::accumulate(m_lengths.data(), m_lengths.data() + idx, 0U);
        return (std::make_pair(m_data.data() + offset, m_lengths[idx]));
    }

    bool operator==(const container& other) const
    {
        return (m_lengths == other.m_lengths && m_data == other.m_data);
    }

    iterator begin() { return (iterator(*this)); }
    const_iterator begin() const { return (iterator(*this)); }

    iterator end() { return (iterator(*this, size())); }
    const_iterator end() const { return (iterator(*this, size())); }
};

} // namespace openperf::packet::generator::traffic::header

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_CONTAINER_HPP_ */
