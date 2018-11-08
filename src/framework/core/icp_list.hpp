#ifndef _ICP_LIST_HPP_
#define _ICP_LIST_HPP_

#include <iterator>
#include <memory>

#include "core/icp_list.h"

namespace icp {

template <typename Item>
class list_iterator {
    icp_list* m_list;
    icp_list_item* m_cursor;

public:
    using tr = std::iterator_traits<Item*>;
    using value_type = typename tr::value_type;
    using pointer   = typename tr::pointer;

    list_iterator(icp_list* list, icp_list_item* cursor) noexcept
        : m_list(list)
        , m_cursor(cursor)
    {}

    list_iterator(icp_list* list)
        : m_list(list)
        , m_cursor(nullptr)
    {}

    value_type operator*() const
    {
        return reinterpret_cast<Item>(icp_list_item_data(m_cursor));
    }

    pointer operator->()
    {
        return reinterpret_cast<Item>(icp_list_item_data(m_cursor));
    }

    list_iterator& operator++()
    {
        icp_list_next(m_list, &m_cursor);
        return *this;
    }

    list_iterator operator++(int)
    {
        auto tmp = m_cursor;
        icp_list_next(m_list, &tmp);
        return (list_iterator(m_list, tmp));
    }

    bool operator!=(const list_iterator& other)
    {
        return (m_cursor != other.m_cursor);
    }
};

template <typename Item>
class list
{
public:
    list()
        : m_list(icp_list_allocate())
    {}

    bool insert(Item* item) { return icp_list_insert(m_list.get(), item); }
    bool remove(Item* item) { return icp_list_delete(m_list.get(), item); }
    size_t size() const { return icp_list_length(m_list.get()); }

    list_iterator<Item*> begin() const
    {
        auto cursor = icp_list_head(m_list.get());
        icp_list_next(m_list.get(), &cursor);
        return list_iterator<Item*>(m_list.get(), cursor);
    }

    list_iterator<Item*> end() const
    {
        return list_iterator<Item*>(m_list.get(), nullptr);
    }

private:
    struct icp_list_deleter {
        void operator()(icp_list *list) const {
            icp_list_free(&list);
        }
    };

    std::unique_ptr<icp_list, icp_list_deleter> m_list;
};

}
#endif /* _ICP_LIST_HPP_ */
