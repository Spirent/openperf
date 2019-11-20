#ifndef _OP_MEMORY_ALLOCATOR_POOL_HPP_
#define _OP_MEMORY_ALLOCATOR_POOL_HPP_

#include <cstddef>
#include <cstdint>

#include <sys/queue.h>

namespace openperf {
namespace memory {
namespace allocator {

class intrusive_stack {
public:
    struct node {
        SLIST_ENTRY(node) next;
    };

    intrusive_stack()
    {
        SLIST_INIT(&m_head);
    }

    void push(node* n)
    {
        SLIST_INSERT_HEAD(&m_head, n, next);
    }

    node* pop()
    {
        if (SLIST_EMPTY(&m_head)) {
            return nullptr;
        }

        auto n = SLIST_FIRST(&m_head);
        SLIST_REMOVE_HEAD(&m_head, next);
        return (n);
    }

private:
    SLIST_HEAD(node_head, node) m_head;
};

class pool
{
    intrusive_stack m_stack;
    uintptr_t m_lower;
    uintptr_t m_upper;
    size_t m_size;

public:
    pool(uintptr_t base, size_t pool_size, size_t item_size);

    void* acquire();
    void  release(const void* ptr);
    size_t size() const noexcept;
    uintptr_t base() const noexcept;
};

}
}
}

#endif /* _OP_MEMORY_ALLOCATOR_POOL_HPP_ */
