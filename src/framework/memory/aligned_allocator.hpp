#ifndef _OP_MEMORY_ALIGNED_ALLOCATOR_HPP_
#define _OP_MEMORY_ALIGNED_ALLOCATOR_HPP_

#include <cstdint>
#include <limits>
#include <new>

namespace openperf::memory {

/**
 * Simple allocator wrapper around posix_memalign.
 */
template <typename T, size_t Alignment> struct aligned_allocator
{
    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be a power of 2");

    // The following will be the same for virtually all allocators.
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;

    template <typename U> struct rebind
    {
        using other = aligned_allocator<U, Alignment>;
    };

    /* This allocator is stateless, so all constructors are useless */
    constexpr aligned_allocator() noexcept = default;
    constexpr aligned_allocator(const aligned_allocator&) noexcept {}

    template <typename U>
    constexpr aligned_allocator(const aligned_allocator<U, Alignment>&) noexcept
    {}

    ~aligned_allocator() = default;

    /* allocators aren't assignable */
    aligned_allocator& operator=(const aligned_allocator&) = delete;

    size_t max_size() const
    {
        return (std::numeric_limits<size_t>::max() / sizeof(value_type));
    }

    pointer allocate(const size_t n) const
    {
        if (n == 0) { return (nullptr); }

        if (n > max_size()) { throw std::bad_array_new_length(); }

        void* ptr = nullptr;
        if (posix_memalign(&ptr, Alignment, n * sizeof(T)) != 0) {
            throw std::bad_alloc();
        }

        return static_cast<pointer>(ptr);
    }

    void deallocate(pointer p, size_t) const { free(p); }
};

/*
 * Since this allocator is stateless, any instances with the same types are
 * effectively equal.
 */
template <typename T, typename U, size_t Alignment>
bool operator==(aligned_allocator<T, Alignment> const&,
                aligned_allocator<U, Alignment> const&) noexcept
{
    return (std::is_same_v<T, U>);
}

template <typename T, typename U, size_t Alignment>
bool operator!=(aligned_allocator<T, Alignment> const&,
                aligned_allocator<U, Alignment> const&) noexcept
{
    return (!(std::is_same_v<T, U>));
}

} // namespace openperf::memory

#endif /* _OP_MEMORY_ALIGNED_ALLOCATOR_HPP_ */
