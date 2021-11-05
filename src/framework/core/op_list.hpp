#ifndef _OP_LIST_HPP_
#define _OP_LIST_HPP_

#include <iterator>
#include <limits>
#include <memory>

#include "core/op_list.h"

namespace openperf {

template <typename Container> class list_iterator
{
    static constexpr size_t invalid_idx = std::numeric_limits<size_t>::max();

    std::reference_wrapper<Container> m_container;
    size_t m_idx = invalid_idx;

public:
    using difference_type = std::ptrdiff_t;
    using value_type = typename Container::value_type;
    using pointer = typename Container::pointer;
    using reference = typename Container::reference;
    using iterator_category = std::forward_iterator_tag;

    list_iterator(Container& container, size_t idx = 0)
        : m_container(container)
        , m_idx(idx)
    {}

    list_iterator(const Container& container, size_t idx = 0)
        : m_container(const_cast<Container&>(container))
        , m_idx(idx)
    {}

    list_iterator(const list_iterator& other)
        : m_container(other.m_container)
        , m_idx(other.m_idx)
    {}

    list_iterator& operator=(const list_iterator& other)
    {
        if (this != &other) {
            m_container = other.m_container;
            m_idx = other.m_idx;
        }
        return (*this);
    }

    bool operator==(const list_iterator& other)
    {
        return (m_idx == other.m_idx);
    }

    bool operator!=(const list_iterator& other) { return (!(*this == other)); }

    list_iterator& operator++()
    {
        m_idx++;
        return (*this);
    }

    list_iterator operator++(int)
    {
        auto to_return = *this;
        m_idx++;
        return (to_return);
    }

    reference operator*() { return *(m_container.get()[m_idx]); }

    pointer operator->() { return (m_container.get()[m_idx]); }
};

template <typename Item> class list_snapshot
{
    Item** m_items; /* an array of Items */
    size_t m_length;

public:
    using value_type = Item;
    using pointer = value_type*;
    using reference = value_type&;
    using iterator = list_iterator<list_snapshot>;
    using const_iterator = const iterator;

    list_snapshot(op_list* list)
    {
        auto error = op_list_snapshot(
            list, reinterpret_cast<void***>(&m_items), &m_length);
        if (error) { throw std::bad_alloc(); }
    }

    ~list_snapshot() { free(m_items); }

    pointer operator[](size_t idx) { return (m_items[idx]); }

    pointer operator[](size_t idx) const { return (m_items[idx]); }

    iterator begin() { return (list_iterator(*this)); };

    const_iterator begin() const { return (list_iterator(*this)); }

    iterator end() { return (list_iterator(*this, m_length)); }

    const_iterator end() const { return (list_iterator(*this, m_length)); }
};

template <typename Item> class list
{
public:
    list()
        : m_list(op_list_allocate())
    {}

    void set_comparator(op_comparator* cmp)
    {
        op_list_set_comparator(m_list.get(), cmp);
    }

    void set_destructor(op_destructor* dest)
    {
        op_list_set_destructor(m_list.get(), dest);
    }

    bool insert(Item* item) { return op_list_insert(m_list.get(), item); }
    bool remove(Item* item) { return op_list_delete(m_list.get(), item); }
    size_t size() const { return op_list_length(m_list.get()); }

    list_snapshot<Item> snapshot()
    {
        return (list_snapshot<Item>(m_list.get()));
    }

    list_snapshot<Item> snapshot() const
    {
        return (list_snapshot<Item>(const_cast<op_list*>(m_list.get())));
    }

    void shrink_to_fit() { op_list_garbage_collect(m_list.get()); }

private:
    struct op_list_deleter
    {
        void operator()(op_list* list) const { op_list_free(&list); }
    };

    std::unique_ptr<op_list, op_list_deleter> m_list;
};

} // namespace openperf
#endif /* _OP_LIST_HPP_ */
