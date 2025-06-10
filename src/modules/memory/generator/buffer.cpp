#include <cerrno>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

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

        /* If can't lock memory, then touch the memory to ensure it is allocated and paged in.
         *
         * Without doing this, read-only tests may see very high read rates.  This is probably
         * because RAM is not actually being accessed and only CPU cache being used.
         * The kernel may be be doing memory compression or may not be allocating memory
         * because it is never written to.
         */
        if (auto error = madvise(m_data, size, MADV_WILLNEED)) {
            OP_LOG(OP_LOG_ERROR,
                   "madvise failed (%zu bytes @ %p): %s\n",
                   size,
                   m_data,
                   strerror(errno));
        }
        const size_t block_size = 8192;
        std::vector<uint8_t> tmp(block_size, 0);
        for (size_t i = 0; i < size; i += block_size) {
            std::memcpy(m_data + i, tmp.data(), std::min(size - i, tmp.size()));
        }
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
