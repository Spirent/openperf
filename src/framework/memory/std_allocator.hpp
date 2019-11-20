#ifndef _OP_MEMORY_STD_ALLOCATOR_HPP_
#define _OP_MEMORY_STD_ALLOCATOR_HPP_

#include <memory>

namespace openperf {
namespace memory {

template <class T, class Impl>
class std_allocator
{
    Impl m_impl;

public:
    using value_type = T;
    using pointer = value_type*;
    using reference = value_type&;
    using const_reference = value_type const&;
    using size_type = size_t;
    //using difference_type = typename std::pointer_traits<pointer>::difference_type;
    //using size_type       = std::make_unsigned_t<difference_type>;

    std_allocator(uintptr_t base, size_t size) noexcept
        : m_impl(base, size, sizeof(T))
    {};

    value_type* allocate(size_t n)
    {
        return static_cast<value_type*>(m_impl.reserve(n * sizeof(value_type)));
    }

    void deallocate(value_type* ptr, size_t) noexcept
    {
        m_impl.release(ptr);
    }

    size_type max_size() const noexcept
    {
        return (m_impl.size() / sizeof(value_type));
    }
};

template <class T, class U, class Impl>
bool operator==(std_allocator<T, Impl> const& lhs, std_allocator<U, Impl> const& rhs) noexcept
{
    return (lhs.base() == rhs.base()
            & lhs.size() == rhs.size());
}

template <class T, class U, class Impl>
bool operator!=(std_allocator<T, Impl> const& x, std_allocator<U, Impl> const& y) noexcept
{
    return !(x == y);
}

}
}

#endif /* _OP_MEMORY_STD_ALLOCATOR_HPP_ */
