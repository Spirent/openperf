#ifndef _OP_MEMORY_ALLOCATOR_FREE_LIST_HPP_
#define _OP_MEMORY_ALLOCATOR_FREE_LIST_HPP_

#include <cstddef>
#include <cstdint>

#include "memory/allocator/bsd_tree.h"

namespace openperf::memory::allocator {
namespace impl {

struct heap_node
{
    size_t size;
    RB_ENTRY(heap_node) entry;
};

RB_HEAD(heap, heap_node);

} // namespace impl

class free_list
{
    constexpr static uint64_t magic_key = 0x05ca1ab1e0c0ffee;
    constexpr static uint64_t alignment = 64;
    impl::heap m_heap;
    uintptr_t m_lower;
    uintptr_t m_upper;

public:
    free_list(uintptr_t heap_base, size_t heap_size, size_t item_size = 0);

    void* reserve(size_t size);
    void release(void* ptr);
    size_t size() const noexcept;
    uintptr_t base() const noexcept;
};

} // namespace openperf::memory::allocator

#endif /* _OP_MEMORY_ALLOCATOR_FREE_LIST_HPP_ */
