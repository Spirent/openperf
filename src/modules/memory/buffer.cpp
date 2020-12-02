#include "buffer.hpp"

#include "framework/core/op_log.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>

namespace openperf::memory::internal {

// Constructors & Destructor
buffer::buffer(mechanism mode)
    : m_mode(mode)
{}

buffer::buffer(std::size_t alignment, mechanism mode)
    : m_mode(mode)
    , m_alignment(alignment)
{
    assert(mode != MMAP);
}

buffer::buffer(std::size_t size, std::size_t alignment, mechanism mode)
    : buffer(alignment, mode)
{
    allocate(size);
}

buffer::buffer(const buffer& other) { *this = other; }

buffer::buffer(buffer&& o) noexcept
    : m_mode(o.m_mode)
    , m_alignment(o.m_alignment)
    , m_ptr(o.m_ptr)
    , m_size(o.m_size)
{
    o.m_ptr = nullptr;
    o.m_size = 0;
}

buffer::~buffer() { release(); }

buffer& buffer::operator=(const buffer& other)
{
    if (this == &other) return *this;

    release();
    m_mode = other.m_mode;
    m_alignment = other.m_alignment;

    if (other.m_size > 0) {
        allocate(other.m_size);

        // NOTE: if statement needs to supress NonNullParamChecker warning
        if (m_ptr != nullptr) std::memcpy(m_ptr, other.m_ptr, m_size);
    }

    return *this;
}

// Methods : public
bool buffer::allocate(std::size_t size)
{
    assert(size > 0);
    assert(m_ptr == nullptr);

    if (m_alignment) return allocate(size, m_alignment.value());

    auto alloc = [this](std::size_t size) -> void* {
        switch (m_mode) {
        case STDLIB:
            return std::malloc(size);
        case MMAP: {
            auto ptr = mmap(nullptr,
                            size,
                            PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE,
                            -1,
                            0);
            return (ptr == MAP_FAILED) ? nullptr : ptr;
        }
        }
    };

    OP_LOG(OP_LOG_INFO, "Allocating buffer of %zu bytes\n", size);

    auto ptr = alloc(size);
    if (ptr == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to allocate %" PRIu64 " byte memory buffer\n",
               size);
        return false;
    }

    m_ptr = ptr;
    m_size = size;
    return true;
}

bool buffer::resize(std::size_t size)
{
    assert(size >= 0);
    if (size == m_size) return true;
    if (size == 0) {
        release();
        return true;
    }

    if (m_size == 0) return allocate(size);
    if (m_alignment) return resize(size, m_alignment.value());

    auto reallocate = [this](std::size_t size) -> void* {
        switch (m_mode) {
        case STDLIB:
            return std::realloc(m_ptr, size);
        case MMAP: {
            auto ptr = mremap(m_ptr, m_size, size, MREMAP_MAYMOVE);
            return (ptr == MAP_FAILED) ? nullptr : ptr;
        }
        }
    };

    OP_LOG(OP_LOG_INFO, "Reallocating buffer (%zu => %zu)\n", m_size, size);

    auto ptr = reallocate(size);
    if (ptr == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to reallocate %" PRIu64 " byte memory buffer\n",
               size);
        return false;
    }

    m_ptr = ptr;
    m_size = size;
    return true;
}

void buffer::release()
{
    if (m_ptr == nullptr) return;

    OP_LOG(OP_LOG_INFO, "Releasing buffer of %zu bytes\n", m_size);

    switch (m_mode) {
    case STDLIB:
        std::free(m_ptr);
        break;
    case MMAP:
        if (munmap(m_ptr, m_size) == -1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed to unallocate %zu"
                   " bytes of memory: %s\n",
                   m_size,
                   strerror(errno));
        }
    }

    m_ptr = nullptr;
    m_size = 0;
}

// Methods : private
bool buffer::allocate(std::size_t size, std::size_t alignment)
{
    assert(size > 0);
    assert(alignment > 0);
    assert(m_ptr == nullptr);

    auto alloc = [this](std::size_t size, std::size_t alignment) -> void* {
        switch (m_mode) {
        case STDLIB:
            return std::aligned_alloc(alignment, size);
        case MMAP: {
            // Not supported
            return nullptr;
        }
        }
    };

    OP_LOG(OP_LOG_INFO,
           "Allocating buffer of %zu with alignment %zu\n",
           size,
           alignment);

    auto ptr = alloc(size, alignment);
    if (ptr == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to allocate %" PRIu64
               " byte memory buffer with alignment %zu\n",
               size,
               alignment);
        return false;
    }

    m_ptr = ptr;
    m_size = size;
    return true;
}

bool buffer::resize(std::size_t size, std::size_t alignment)
{
    assert(alignment > 0);
    if (size == m_size) return true;

    auto alloc = [this](std::size_t size, std::size_t alignment) -> void* {
        switch (m_mode) {
        case STDLIB:
            return std::aligned_alloc(alignment, size);
        case MMAP: {
            // Not supported
            return nullptr;
        }
        }
    };

    OP_LOG(OP_LOG_INFO,
           "Reallocating buffer of %zu with alignment %zu\n",
           size,
           alignment);

    auto ptr = alloc(size, alignment);
    if (ptr == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to reallocate %" PRIu64
               " byte memory buffer with alignment %zu\n",
               size,
               alignment);
        return false;
    }

    std::memcpy(ptr, m_ptr, std::min(m_size, size));
    release();

    m_ptr = ptr;
    m_size = size;
    return true;
}

} // namespace openperf::memory::internal
