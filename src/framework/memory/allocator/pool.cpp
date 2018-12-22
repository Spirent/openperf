#include <algorithm>
#include <new>
#include <stdexcept>

#include "memory/allocator/pool.h"

namespace icp {
namespace memory {
namespace allocator {

static __attribute__((const)) bool power_of_two(uint64_t x) {
    return !(x & (x - 1));
}

static uint64_t __attribute__((const)) next_power_of_two(uint64_t x) {
    if (power_of_two(x)) { return x; }
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

pool::pool(uintptr_t base, size_t pool_size, size_t item_size)
    : m_lower(base)
    , m_upper(base + pool_size)
    , m_size(next_power_of_two(std::max(item_size, sizeof(intrusive_stack::node))))
{
    if (pool_size < m_size) {
        throw std::runtime_error("pool_size is too small for item_size");
    }
    for (size_t offset = 0; offset + m_size < pool_size; offset += m_size) {
        auto node = reinterpret_cast<intrusive_stack::node*>(base + offset);
        m_stack.push(node);
    }
}

void* pool::acquire()
{
    return (reinterpret_cast<void*>(m_stack.pop()));
}

void pool::release(const void* ptr)
{
    if (reinterpret_cast<uintptr_t>(ptr) < m_lower
        || reinterpret_cast<uintptr_t>(ptr) > m_upper - m_size) {
        throw std::runtime_error("ptr is out of range");
    }

    m_stack.push(
        reinterpret_cast<intrusive_stack::node*>(
            const_cast<void*>(ptr)));
}

size_t pool::size() const noexcept
{
    return (m_upper - m_lower);
}

uintptr_t pool::base() const noexcept
{
    return (m_lower);
}

}
}
}
