#ifndef _OP_MEMORY_OFFSET_PTR_H_
#define _OP_MEMORY_OFFSET_PTR_H_

/**
 * Pointer that contains the offset from this pointer to the pointed to value
 * This is specifically useful for referencing addresses in shared memory
 * Inspired by boost:inteprocess::offset_ptr
 */

#include <cassert>
#include <cstddef>
#include <iterator>

namespace openperf::memory {

template <typename T>
class offset_ptr {
    /*
     * Magic number to determine if we point to null or not.  One byte away
     * from our address is still us, so this can never be a valid value.
     */
    static constexpr int null_offset = 1;

    ptrdiff_t m_offset;  /* the offset of whatever we are pointing to */

    ptrdiff_t offset_of(T* ptr)
    {
        assert(ptr);
        return (reinterpret_cast<intptr_t>(ptr) - reinterpret_cast<intptr_t>(this));
    }

    struct no_init{};
    offset_ptr(no_init) {}

public:
    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef std::random_access_iterator_tag iterator_category;

    static offset_ptr uninitialized() { return offset_ptr(no_init()); }

    offset_ptr(pointer ptr = nullptr)
        : m_offset(ptr ? offset_of(ptr) : null_offset)
    {}

    offset_ptr(const offset_ptr& optr)
        : m_offset(offset_of(optr.get()))
    {}

    offset_ptr& operator=(T* ptr) noexcept
    {
        m_offset = (ptr ? offset_of(ptr) : null_offset);
        return (*this);
    }

    offset_ptr& operator=(const offset_ptr& optr) noexcept
    {
        m_offset = offset_of(optr.get());
        return (*this);
    }

    pointer get() const noexcept
    {
        return (reinterpret_cast<T*>(m_offset == null_offset
                                     ? reinterpret_cast<intptr_t>(nullptr)
                                     : reinterpret_cast<intptr_t>(this) + m_offset));
    }

    pointer operator->() const noexcept
    {
        return (get());
    }

    reference operator *() const noexcept
    {
        return (*get());
    }

    reference operator[](size_t idx) const noexcept
    {
        return (get()[idx]);
    }

    bool operator==(const pointer ptr) const noexcept
    {
        return (get() == ptr);
    }

    bool operator==(const offset_ptr& optr) const noexcept
    {
        return (get() == optr.get());
    }

    explicit operator bool() const noexcept
    {
        return (m_offset != null_offset);
    }

    bool operator!() const noexcept
    {
        return (m_offset == null_offset);
    }
};

}

#endif /* _OP_MEMORY_OFFSET_PTR_H_ */
