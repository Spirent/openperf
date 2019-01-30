#include <cassert>
#include <cerrno>
#include <cstring>
#include <numeric>
#include <string>
#include <unistd.h>

#include "tl/expected.hpp"

#include "socket/circular_buffer.h"

namespace icp {
namespace socket {

template <typename T>
static __attribute__((const)) T align_up(T x, T align)
{
    return ((x + align - 1) & ~(align - 1));
}

static tl::expected<uint8_t*, int> create_double_buffer(int fd, size_t size)
{
    /* Get enough memory to map two copies of the buffer */
    auto buffer = reinterpret_cast<uint8_t*>(
        mmap(NULL, size << 1, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
    if (buffer == MAP_FAILED) {
        return (tl::make_unexpected(errno));
    }

    /* Map the first copy into place */
    auto map_result = mmap(buffer, size, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_FIXED, fd, 0);
    if (map_result == MAP_FAILED) {
        return (tl::make_unexpected(errno));
    }

    /* Map the second copy into place */
    map_result = mmap(buffer + size, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_FIXED, fd, 0);
    if (map_result == MAP_FAILED) {
        return (tl::make_unexpected(errno));
    }

    return (buffer);
}

circular_buffer::circular_buffer(size_t initial_size)
    : m_size(align_up(initial_size, page_size))
    , m_read(0)
    , m_write(0)
{
    /* Create a temporary file which we'll map into memory */
    std::string name = "/tmp/circular_buffer-XXXXXXXX";
    auto fd = mkstemp(name.data());
    if (fd == -1) {
        throw std::runtime_error("Could not create backing file: "
                                 + std::string(strerror(errno)));
    }

    /* Resize it */
    auto error = ftruncate(fd, m_size);
    if (error) {
        close(fd);
        unlink(name.c_str());
        throw std::runtime_error("Could not resize backing file: "
                                 + std::string(strerror(errno)));
    }

    /* Map the two copyies of the file into memory */
    auto result = create_double_buffer(fd, m_size);
    close(fd);  /* no longer needed */
    unlink(name.c_str());
    if (!result) {
        throw std::runtime_error("Could not mmap file into double buffer: "
                                 + std::string(strerror(result.error())));
    }

    m_buffer = *result;
}

circular_buffer::circular_buffer() {}

circular_buffer::~circular_buffer()
{
    if (m_buffer) munmap(m_buffer, m_size << 1);
}

circular_buffer& circular_buffer::operator=(circular_buffer&& other)
{
    if (this != &other) {
        m_size = other.m_size;
        m_read.store(other.m_read.load(std::memory_order_acquire), std::memory_order_release);
        m_write.store(other.m_write.load(std::memory_order_acquire), std::memory_order_release);
        m_buffer = other.m_buffer;
        other.m_buffer = nullptr;
    }
    return (*this);
}

circular_buffer::circular_buffer(circular_buffer&& other)
    : m_buffer(other.m_buffer)
    , m_size(other.m_size)
    , m_read(other.m_read.load(std::memory_order_acquire))
    , m_write(other.m_write.load(std::memory_order_acquire))
{
    other.m_buffer = nullptr;
}

size_t circular_buffer::readable() const
{
    auto read_cursor = read();
    auto write_cursor = write();

    return (read_cursor <= write_cursor
            ? write_cursor - read_cursor
            : m_size - (read_cursor - write_cursor));
}

size_t circular_buffer::writable() const
{
    return (m_size - readable() - 1);
}

bool circular_buffer::empty() const
{
    return (read() == write());
}

bool circular_buffer::full() const
{
    return (read() == (write() + 1));
}

void* circular_buffer::peek()
{
    return (reinterpret_cast<void*>(m_buffer + m_read.load(std::memory_order_acquire)));
}

size_t circular_buffer::skip(size_t length)
{
    auto to_skip = std::min(readable(), length);
    if (!to_skip) {
        return (0);
    }

    auto cursor = m_read.load(std::memory_order_acquire);
    m_read.store((cursor + to_skip > m_size
                  ? cursor + to_skip - m_size
                  : cursor + to_skip),
                 std::memory_order_release);
    return (to_skip);
}

size_t circular_buffer::read(void* ptr, size_t length)
{
    auto to_read = std::min(readable(), length);
    if (!to_read) {
        return (0);
    }

    auto cursor = m_read.load(std::memory_order_acquire);
    memcpy(ptr, m_buffer + cursor, to_read);
    m_read.store((cursor + to_read > m_size
                  ? cursor + to_read - m_size
                  : cursor + to_read),
                 std::memory_order_release);

    return (to_read);
}

size_t circular_buffer::read(iovec iov[], size_t iovcnt)
{
    size_t total = 0;
    for (size_t i = 0; i < iovcnt; i++) {
        auto consumed = read(iov[i].iov_base, iov[i].iov_len);
        total += consumed;
        if (consumed < iov[i].iov_len) break;  /* we're done */
    }
    return (total);
}

tl::expected<size_t, int> circular_buffer::read(pid_t pid, iovec iov[], size_t iovcnt)
{
    auto cursor = read();
    iovec readvec = {
        .iov_base = m_buffer + cursor,
        .iov_len = readable()
    };

    auto result = process_vm_readv(pid, iov, iovcnt, &readvec, 1, 0);
    if (result == -1) return (tl::make_unexpected(errno));

    m_read.store((cursor + result > m_size
                  ? cursor + result - m_size
                  : cursor + result),
                 std::memory_order_release);
    return (result);
}

size_t circular_buffer::write(const void* ptr, size_t length)
{
    auto to_write = std::min(writable(), length);
    if (!to_write) {
        return (0);
    }

    auto cursor = write();
    memcpy(m_buffer + cursor, ptr, to_write);
    m_write.store((cursor + to_write > m_size
                   ? cursor + to_write - m_size
                   : cursor + to_write),
                  std::memory_order_release);
    return (to_write);
}

size_t circular_buffer::write(const iovec iov[], size_t iovcnt)
{
    size_t total = 0;
    for (size_t i = 0; i < iovcnt; i++) {
        auto written = write(iov[i].iov_base, iov[i].iov_len);
        total += written;
        if (written < iov[i].iov_len) break;  /* we're done */
    }
    return (total);
}

tl::expected<size_t, int> circular_buffer::write(pid_t pid, const iovec iov[], size_t iovcnt)
{
    auto cursor = write();
    iovec writevec = {
        .iov_base = m_buffer + cursor,
        .iov_len = writable()
    };

    auto result = process_vm_writev(pid, iov, iovcnt, &writevec, 1, 0);
    if (result == -1) return (tl::unexpected(errno));

    m_write.store((cursor + result > m_size
                   ? cursor + result - m_size
                   : cursor + result),
                  std::memory_order_release);
    return (result);
}

}
}
