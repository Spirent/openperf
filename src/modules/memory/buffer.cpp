#include "buffer.hpp"

#include "framework/core/op_log.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

namespace openperf::memory::internal {

// Constructors & Destructor
buffer::buffer(std::size_t alignment)
    : m_alignment(alignment)
{}

buffer::buffer(std::size_t size, std::size_t alignment)
    : buffer(alignment)
{
    allocate(size);
}

buffer::buffer(const buffer& other) { *this = other; }

buffer::buffer(buffer&& other) noexcept
    : m_alignment(other.m_alignment)
    , m_ptr(other.m_ptr)
    , m_size(other.m_size)
{
    other.m_ptr = nullptr;
    other.m_size = 0;
}

buffer::~buffer() { release(); }

buffer& buffer::operator=(const buffer& other)
{
    if (this == &other) return *this;

    release();
    m_alignment = other.m_alignment;

    if (other.m_size > 0) {
        allocate(other.m_size);

        // NOTE: if statement needs to supress NonNullParamChecker warning
        if (m_ptr != nullptr) std::memcpy(m_ptr, other.m_ptr, m_size);
    }

    return *this;
}

// Methods : public
void buffer::allocate(std::size_t size)
{
    assert(size > 0);
    assert(m_ptr == nullptr);

    if (m_alignment) return allocate(size, m_alignment.value());

    OP_LOG(OP_LOG_TRACE, "Allocating buffer of %zu bytes\n", size);

    auto ptr = std::malloc(size);
    if (ptr == nullptr) {
        OP_LOG(
            OP_LOG_ERROR, "Failed to allocate %zu bytes memory buffer\n", size);
        throw std::bad_alloc();
    }

    m_ptr = ptr;
    m_size = size;
}

void buffer::resize(std::size_t size)
{
    assert(size >= 0);
    if (size == m_size) return;

    if (size == 0) return release();
    if (m_size == 0) return allocate(size);
    if (m_alignment) return resize(size, m_alignment.value());

    OP_LOG(OP_LOG_TRACE, "Resizing buffer (%zu => %zu)\n", m_size, size);

    auto ptr = std::realloc(m_ptr, size);
    if (ptr == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to resize (%zu => %zu) memory buffer\n",
               m_size,
               size);
        throw std::bad_alloc();
    }

    m_ptr = ptr;
    m_size = size;
}

void buffer::release()
{
    if (m_ptr == nullptr) return;

    OP_LOG(OP_LOG_TRACE, "Releasing buffer of %zu bytes\n", m_size);

    std::free(m_ptr);

    m_ptr = nullptr;
    m_size = 0;
}

// Methods : private
void buffer::allocate(std::size_t size, std::size_t alignment)
{
    assert(size > 0);
    assert(alignment > 0);
    assert(m_ptr == nullptr);

    OP_LOG(OP_LOG_TRACE,
           "Allocating buffer of %zu with alignment %zu\n",
           size,
           alignment);

    auto ptr = std::aligned_alloc(alignment, size);
    if (ptr == nullptr) {
        OP_LOG(
            OP_LOG_ERROR,
            "Failed to allocate %zu bytes memory buffer with alignment %zu\n",
            size,
            alignment);
        throw std::bad_alloc();
    }

    m_ptr = ptr;
    m_size = size;
}

void buffer::resize(std::size_t size, std::size_t alignment)
{
    assert(alignment > 0);
    if (size == m_size) return;

    OP_LOG(OP_LOG_TRACE,
           "Resizing buffer (%zu => %zu) with alignment %zu\n",
           m_size,
           size,
           alignment);

    auto ptr = std::aligned_alloc(alignment, size);
    if (ptr == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to resize (%zu => %zu) buffer with alignment %zu\n",
               m_size,
               size,
               alignment);
        throw std::bad_alloc();
    }

    std::memcpy(ptr, m_ptr, std::min(m_size, size));
    std::free(m_ptr);

    m_ptr = ptr;
    m_size = size;
}

} // namespace openperf::memory::internal
