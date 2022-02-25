#include <cerrno>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>

#include "sys/mman.h"

#include "core/op_log.h"
#include "memory/generator/buffer.hpp"

namespace openperf::memory::generator {

buffer::buffer(size_t size)
    : m_data(static_cast<std::byte*>(mmap(nullptr,
                                          size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS,
                                          -1,
                                          0)))
    , m_length(size)
{
    if (m_data == MAP_FAILED) {
        fprintf(stderr, "mmap failed! %s\n", strerror(errno));
        fflush(stderr);
        throw std::bad_alloc();
    }

    /* Lock the buffer to ensure it stays in memory */
    if (auto error = mlock(m_data, size)) {
        OP_LOG(OP_LOG_WARNING,
               "Could not lock memory buffer; do you need to increase your "
               "resource limit?\n");
    }

    OP_LOG(OP_LOG_DEBUG,
           "Created mmap buffer (%zu bytes @ %p)\n",
           m_length,
           m_data);
}

buffer::~buffer()
{
    /* Unlocking is probably unnecessary, but I like symmetry. */
    if (munlock(m_data, m_length) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Could not unlock memory (%zu bytes @ %p): %s\n",
               m_length,
               m_data,
               strerror(errno));
    }

    if (munmap(m_data, m_length) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Could not unmap memory (%zu bytes @ %p): %s\n",
               m_length,
               m_data,
               strerror(errno));
    }
}

std::byte* buffer::data() { return (m_data); }

const std::byte* buffer::data() const { return (m_data); }

size_t buffer::length() const { return (m_length); }

} // namespace openperf::memory::generator
