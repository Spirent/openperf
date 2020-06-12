#ifndef _OP_ANALYZER_STATISTICS_FLOW_HEADER_VIEW_HPP_
#define _OP_ANALYZER_STATISTICS_FLOW_HEADER_VIEW_HPP_

#include <functional>
#include <variant>
#include <vector>

#include "packet/analyzer/statistics/flow/header.hpp"
#include "packet/protocol/protocols.hpp"
#include "packetio/packet_type.hpp"

namespace openperf::packet::analyzer::statistics::flow {

template <typename Container> class header_iterator
{
    std::reference_wrapper<Container> m_container;
    size_t m_idx = std::numeric_limits<size_t>::max();

public:
    using difference_type = std::ptrdiff_t;
    using value_type = typename Container::header_variant;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;

    header_iterator(Container& container, size_t idx = 0)
        : m_container(container)
        , m_idx(idx)
    {}

    header_iterator(const Container& container, size_t idx = 0)
        : m_container(const_cast<Container&>(container))
        , m_idx(idx)
    {}

    bool operator==(const header_iterator& other)
    {
        return (m_idx == other.m_idx);
    }

    bool operator!=(const header_iterator& other)
    {
        return (!(*this == other));
    }

    header_iterator& operator++()
    {
        m_idx++;
        return (*this);
    }

    header_iterator operator++(int)
    {
        auto to_return = *this;
        m_idx++;
        return (to_return);
    }

    value_type operator*() { return (m_container.get()[m_idx]); }
};

class header_view
{
public:
    using header_variant = std::variant<const libpacket::protocol::ethernet*,
                                        const libpacket::protocol::ipv4*,
                                        const libpacket::protocol::ipv6*,
                                        const libpacket::protocol::mpls*,
                                        const libpacket::protocol::tcp*,
                                        const libpacket::protocol::udp*,
                                        const libpacket::protocol::vlan*,
                                        std::basic_string_view<uint8_t>>;
    using iterator = header_iterator<header_view>;
    using const_iterator = const iterator;

    header_view(const header& header);

    size_t size() const;

    header_variant operator[](size_t idx) const;

    const_iterator begin() const;
    const_iterator end() const;

private:
    std::vector<header_variant> m_headers;
};

} // namespace openperf::packet::analyzer::statistics::flow

#endif /* _OP_ANALYZER_STATISTICS_FLOW_HEADER_VIEW_HPP_ */
