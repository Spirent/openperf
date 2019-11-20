#include <algorithm>
#include <cassert>
#include <new>
#include <stdexcept>

#include "memory/allocator/free_list.hpp"

namespace openperf {
namespace memory {
namespace allocator {

namespace impl {

/**
 * Using the bsd tree code here is nice, because it allows us to use
 * the memory heap to store (nearly) all of it's own meta data.
 */
struct heap_tag {
    size_t size;
    uint64_t magic;
    uint64_t pad[6];
};

/* sort by address */
static int compare_heap_nodes(heap_node* lhs, heap_node* rhs)
{
    auto left = reinterpret_cast<uintptr_t>(lhs);
    auto right = reinterpret_cast<uintptr_t>(rhs);

    return (left < right ? -1
            : left > right ? 1
            : 0);
}

RB_GENERATE_STATIC(heap, heap_node, entry, compare_heap_nodes);

/* Find the node with the least amount of wasted space */
heap_node* find_free_node(heap* h, size_t size)
{
    heap_node* to_return = nullptr;
    heap_node* current = nullptr;
    RB_FOREACH(current, heap, h) {
        if (current->size < size) continue;  /* to small */
        if (to_return == nullptr
            || current->size < to_return->size) {
            to_return = current;
        }
    }

    return (to_return
            ? RB_REMOVE(heap, h, to_return)
            : nullptr);
}

}

static __attribute__((const)) uint64_t align_up(uint64_t x, uint64_t align)
{
    return ((x + align - 1) & ~(align - 1));
}

free_list::free_list(uintptr_t heap_base, size_t heap_size,
                     size_t item_size __attribute__((unused)))
    : m_lower(heap_base)
    , m_upper(heap_base + heap_size)
{
    RB_INIT(&m_heap);
    auto node = reinterpret_cast<impl::heap_node*>(heap_base);
    node->size = heap_size;
    RB_INSERT(heap, &m_heap, node);
}

void* free_list::reserve(size_t size)
{
    /*
     * We cannot allocate below the size of our meta data, so bump the
     * allocation size up if needed.
     */
    size = align_up(std::max(size + sizeof(impl::heap_tag),
                             sizeof(impl::heap_node)),
                    alignment);
    auto node = impl::find_free_node(&m_heap, size);
    if (node == nullptr) {
        throw std::bad_alloc();  /* oom or request too big */
    }

    /**
     * Check if we need to split this node up.
     * Obviously, we need more than a header_node extra available, otherwise
     * we're just wasting space.
     */
    if (node->size > size + sizeof(impl::heap_node)) {
        auto extra = reinterpret_cast<impl::heap_node*>(
            reinterpret_cast<uintptr_t>(node) + size);
        extra->size = node->size - size;
        RB_INSERT(heap, &m_heap, extra);
    }

    /* Tag this memory before releasing to the client */
    auto tag = reinterpret_cast<impl::heap_tag*>(node);
    tag->size = size;
    tag->magic = magic_key;

    return reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(tag) + sizeof(impl::heap_tag));
}

void free_list::release(void* ptr)
{
    /* Check address range */
    if (reinterpret_cast<uintptr_t>(ptr) < m_lower
        || reinterpret_cast<uintptr_t>(ptr) > m_upper) {
        throw std::runtime_error("invalid range for ptr");
    }

    /* Check tag & magic key */
    auto tag = reinterpret_cast<impl::heap_tag*>(
        reinterpret_cast<uintptr_t>(ptr) - sizeof(impl::heap_tag));
    if (tag->magic != magic_key) {
        throw std::runtime_error("memory corruption for ptr");
    }

    auto node = reinterpret_cast<impl::heap_node*>(tag);
    assert(node->size == tag->size);  /* sanity check congruent structs */
    assert(node->size != 1);
    auto ret = RB_INSERT(heap, &m_heap, node);
    if (ret != nullptr) {
        throw std::runtime_error("Double free!?!");
    }

    /* Finally, see if we can merge this node into it's neighbors */
    auto next = RB_NEXT(heap, &m_heap, node);
    if (next
        && reinterpret_cast<uintptr_t>(node) + node->size == reinterpret_cast<uintptr_t>(next)) {
        node->size += next->size;
        RB_REMOVE(heap, &m_heap, next);
    }

    auto prev = RB_PREV(heap, &m_heap, node);
    if (prev
        && reinterpret_cast<uintptr_t>(prev) + prev->size == reinterpret_cast<uintptr_t>(node)) {
        prev->size += node->size;
        RB_REMOVE(heap, &m_heap, node);
    }
}

size_t free_list::size() const noexcept
{
    return (m_upper - m_lower - sizeof(impl::heap_tag));
}

uintptr_t free_list::base() const noexcept
{
    return (m_lower);
}

}
}
}
