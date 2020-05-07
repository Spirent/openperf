#ifndef _OP_MEMORY_STD_ALLOCATOR_HPP_
#define _OP_MEMORY_STD_ALLOCATOR_HPP_

#include <cstddef>
#include <cstdint>
#include <new>

namespace openperf::memory {

template <class T, class Impl> class std_allocator
{
    Impl m_impl;

public:
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using is_always_equal = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;

    template <typename U> struct rebind
    {
        using other = std_allocator<U, Impl>;
    };

    std_allocator(uintptr_t base, size_t size) noexcept
        : m_impl(base, size, sizeof(T)){};

    ~std_allocator() = default;

    std_allocator& operator=(const std_allocator&) = delete;

    size_type max_size() const noexcept
    {
        return (m_impl.size() / sizeof(value_type));
    }

    pointer allocate(size_t n)
    {
        if (n == 0) { return (nullptr); }

        if (n > max_size()) { throw std::bad_array_new_length(); }

        auto ptr = m_impl.reserve(n * sizeof(value_type));
        if (!ptr) { throw std::bad_alloc(); }
        return (static_cast<pointer>(ptr));
    }

    void deallocate(value_type* ptr, size_t) noexcept { m_impl.release(ptr); }
};

template <class T, class U, class Impl>
bool operator==(std_allocator<T, Impl> const& lhs,
                std_allocator<U, Impl> const& rhs) noexcept
{
    return (lhs.base() == rhs.base() & lhs.size() == rhs.size());
}

template <class T, class U, class Impl>
bool operator!=(std_allocator<T, Impl> const& x,
                std_allocator<U, Impl> const& y) noexcept
{
    return !(x == y);
}

} // namespace openperf::memory

#endif /* _OP_MEMORY_STD_ALLOCATOR_HPP_ */
