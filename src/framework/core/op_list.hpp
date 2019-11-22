#ifndef _OP_LIST_HPP_
#define _OP_LIST_HPP_

#include <iterator>
#include <memory>

#include "core/op_list.h"

namespace openperf {

template <typename Item> class list_iterator
{
    op_list* m_list;
    op_list_item* m_cursor;

public:
    using tr = std::iterator_traits<Item*>;
    using value_type = typename tr::value_type;
    using pointer = typename tr::pointer;

    list_iterator(op_list* list, op_list_item* cursor) noexcept
        : m_list(list)
        , m_cursor(cursor)
    {}

    list_iterator(op_list* list)
        : m_list(list)
        , m_cursor(nullptr)
    {}

    value_type operator*() const
    {
        return reinterpret_cast<Item>(op_list_item_data(m_cursor));
    }

    pointer operator->()
    {
        return reinterpret_cast<Item>(op_list_item_data(m_cursor));
    }

    list_iterator& operator++()
    {
        op_list_next(m_list, &m_cursor);
        return *this;
    }

    list_iterator operator++(int)
    {
        auto tmp = m_cursor;
        op_list_next(m_list, &tmp);
        return (list_iterator(m_list, tmp));
    }

    bool operator!=(const list_iterator& other)
    {
        return (m_cursor != other.m_cursor);
    }
};

template <typename Item> class list
{
public:
    list()
        : m_list(op_list_allocate())
    {}

    bool insert(Item* item) { return op_list_insert(m_list.get(), item); }
    bool remove(Item* item) { return op_list_delete(m_list.get(), item); }
    size_t size() const { return op_list_length(m_list.get()); }

    list_iterator<Item*> begin() const
    {
        auto cursor = op_list_head(m_list.get());
        op_list_next(m_list.get(), &cursor);
        return list_iterator<Item*>(m_list.get(), cursor);
    }

    list_iterator<Item*> end() const
    {
        return list_iterator<Item*>(m_list.get(), nullptr);
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
