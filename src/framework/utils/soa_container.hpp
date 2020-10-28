#ifndef _OP_UTILS_SOA_CONTAINER_HPP_
#define _OP_UTILS_SOA_CONTAINER_HPP_

#include <cstddef>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

namespace openperf::utils {

template <template <typename...> typename Container, typename Item>
struct soa_container_adapter
{};

template <template <typename...> typename Container,
          template <typename...>
          typename Item,
          typename... Types>
struct soa_container_adapter<Container, Item<Types...>>
{
    using container_type = std::tuple<Container<Types>...>;
    using value_type = Item<Types...>;

    static constexpr void
    set(container_type& container, size_t idx, value_type&& value)
    {
        set_impl(container,
                 idx,
                 std::forward<value_type>(value),
                 std::make_index_sequence<sizeof...(Types)>());
    }

    static constexpr value_type get(container_type& container, size_t idx)
    {
        return (get_impl(
            container, idx, std::make_index_sequence<sizeof...(Types)>()));
    }

    static constexpr void reserve(container_type& container, size_t size)
    {
        reserve_impl(
            container, size, std::make_index_sequence<sizeof...(Types)>());
    }

    static constexpr void push_back(container_type& container,
                                    value_type&& value)
    {
        push_back_impl(container,
                       std::forward<value_type>(value),
                       std::make_index_sequence<sizeof...(Types)>());
    }

    static constexpr bool empty(const container_type& container)
    {
        return (std::get<0>(container).empty());
    }

    static constexpr size_t size(const container_type& container)
    {
        return (std::get<0>(container).size());
    }

    static constexpr size_t max_size(const container_type& container)
    {
        return (std::get<0>(container).max_size());
    }

    template <size_t Idx>
    static constexpr typename std::tuple_element_t<Idx, std::tuple<Types...>>*
    data(container_type& container)
    {
        return (std::get<Idx>(container).data());
    }

    template <size_t Idx>
    static constexpr const auto& get(const container_type& container)
    {
        return (std::get<Idx>(container));
    }

    friend bool operator==(const container_type& left,
                           const container_type& right)
    {
        return (equals_impl(
            left, right, std::make_index_sequence<sizeof...(Types)>()));
    }

private:
    template <size_t... Indexes>
    static constexpr auto get_impl(container_type& container,
                                   size_t idx,
                                   std::index_sequence<Indexes...>)
    {
        return (value_type{
            std::reference_wrapper(std::get<Indexes>(container)[idx])...});
    }

    template <size_t Index>
    static constexpr void
    set_impl_setter(container_type& container, size_t idx, value_type&& value)
    {
        std::get<Index>(container)[idx] =
            std::get<Index>(std::forward<value_type>(value));
    }

    template <size_t... Indexes>
    static constexpr void set_impl(container_type& container,
                                   size_t idx,
                                   value_type&& value,
                                   std::index_sequence<Indexes...>)
    {
        (set_impl_setter<Indexes>(
             container, idx, std::forward<value_type>(value)),
         ...);
    }

    template <size_t... Indexes>
    static constexpr void reserve_impl(container_type& container,
                                       size_t size,
                                       std::index_sequence<Indexes...>)
    {
        (std::get<Indexes>(container).reserve(size), ...);
    }

    template <size_t... Indexes>
    static constexpr void push_back_impl(container_type& container,
                                         value_type&& value,
                                         std::index_sequence<Indexes...>)
    {
        (std::get<Indexes>(container).push_back(
             std::get<Indexes>(std::forward<value_type>(value))),
         ...);
    }

    template <size_t... Indexes>
    static constexpr bool equals_impl(const container_type& left,
                                      const container_type& right,
                                      std::index_sequence<Indexes...>)
    {
        return ((std::get<Indexes>(left) == std::get<Indexes>(right)) && ...);
    }
};

template <typename Container> class soa_container_iterator
{
    static constexpr size_t invalid_idx = std::numeric_limits<size_t>::max();
    Container* m_container = nullptr;
    size_t m_idx = invalid_idx;

public:
    using adapter = typename Container::adapter;
    using difference_type = std::ptrdiff_t;
    using value_type = typename adapter::value_type;
    using pointer = void;
    using reference = value_type&;
    using iterator_category = std::bidirectional_iterator_tag;

    soa_container_iterator(Container* container, size_t idx = 0)
        : m_container(container)
        , m_idx(idx)
    {}

    soa_container_iterator(const Container* container, size_t idx = 0)
        : m_container(const_cast<Container*>(container))
        , m_idx(idx)
    {}

    soa_container_iterator(const soa_container_iterator& other)
        : m_container(other.m_container)
        , m_idx(other.m_idx)
    {}

    soa_container_iterator& operator=(soa_container_iterator const& other)
    {
        m_container = other.m_container;
        m_idx = other.m_idx;
    }

    soa_container_iterator& operator=(std::nullptr_t const&)
    {
        m_container = nullptr;
        m_idx = invalid_idx;
    }

    friend bool operator==(soa_container_iterator const& left,
                           soa_container_iterator const& right)
    {
        return (left.m_container == right.m_container
                && left.m_idx == right.m_idx);
    }

    friend bool operator!=(soa_container_iterator const& left,
                           soa_container_iterator const& right)
    {
        return (!(left == right));
    }

    template <typename T>
    typename std::enable_if_t<std::is_integral_v<T>> operator+=(T incr)
    {
        m_idx += incr;
    }

    template <typename T>
    typename std::enable_if_t<std::is_integral_v<T>> operator-=(T decr)
    {
        m_idx -= decr;
    }

    void operator++() { operator+=(1); }
    void operator--() { operator-=(1); }

    operator bool() const
    {
        return (m_container && m_idx >= m_container->size());
    }

    value_type operator*() { return ((*m_container)[m_idx]); }
};

template <template <typename...> typename Container, typename Item>
struct soa_container
{
    using adapter = soa_container_adapter<Container, Item>;
    using iterator = soa_container_iterator<soa_container<Container, Item>>;
    using const_iterator = const iterator;
    using difference_type = std::ptrdiff_t;
    using value_type = typename adapter::value_type;
    using reference_type = value_type&;
    using size_type = size_t;

    void push_back(value_type&& value)
    {
        adapter::push_back(m_adapter, std::forward<value_type>(value));
    }

    void set(size_t idx, value_type&& value)
    {
        adapter::set(m_adapter, idx, std::forward<value_type>(value));
    }

    bool empty() const { return (adapter::empty(m_adapter)); }

    size_t size() const { return (adapter::size(m_adapter)); }

    size_t max_size() const { return (adapter::max_size(m_adapter)); }

    value_type operator[](size_t idx) { return (adapter::get(m_adapter, idx)); }

    void reserve(size_t size) { adapter::reserve(m_adapter, size); }

    template <size_t Idx> std::tuple_element_t<Idx, value_type>* data()
    {
        return (adapter::template data<Idx>(m_adapter));
    }

    template <size_t Idx> const auto& get() const
    {
        return (adapter::template get<Idx>(m_adapter));
    }

    iterator begin() { return (iterator(this)); }
    const_iterator begin() const { return iterator((this)); }
    const_iterator cbegin() const { return (begin()); }

    iterator end() { return (iterator(this, size())); }
    const_iterator end() const { return (iterator(this, size())); }
    const_iterator cend() const { return (end()); }

    friend bool operator==(const soa_container& left,
                           const soa_container& right)
    {
        return (left.m_adapter == right.m_adapter);
    }

    friend bool operator!=(const soa_container& left,
                           const soa_container& right)
    {
        return (!(left.m_adapter == right.m_adapter));
    }

private:
    typename adapter::container_type m_adapter;
};

} // namespace openperf::utils

#endif /* _OP_UTILS_SOA_CONTAINER_HPP_ */
