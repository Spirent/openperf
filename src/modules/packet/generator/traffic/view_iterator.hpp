#ifndef _OP_PACKET_GENERATOR_TRAFFIC_VIEW_ITERATOR_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_VIEW_ITERATOR_HPP_

#include <functional>
#include <iterator>
#include <limits>

namespace openperf::packet::generator::traffic {

template <typename Container> class view_iterator
{
    static constexpr size_t invalid_idx = std::numeric_limits<size_t>::max();

    /*
     * We use a reference wrapper to prevent the assignment operator
     * below from making a copy.
     */
    std::reference_wrapper<Container> m_container;
    size_t m_idx = invalid_idx;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = typename Container::view_type;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;

    view_iterator(Container& container, size_t idx = 0)
        : m_container(container)
        , m_idx(idx)
    {}

    view_iterator(const Container& container, size_t idx = 0)
        : m_container(const_cast<Container&>(container))
        , m_idx(idx)
    {}

    view_iterator& operator=(const view_iterator& other)
    {
        m_container = other.m_container;
        m_idx = other.m_idx;
        return (*this);
    }

    bool operator==(const view_iterator& other)
    {
        return (m_idx == other.m_idx);
    }

    bool operator!=(const view_iterator& other) { return (!(*this == other)); }

    view_iterator& operator++()
    {
        m_idx++;
        return (*this);
    }

    view_iterator operator++(int)
    {
        auto to_return = *this;
        m_idx++;
        return (to_return);
    }

    value_type operator*() { return (m_container.get()[m_idx]); }
};

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_VIEW_ITERATOR_HPP_ */
