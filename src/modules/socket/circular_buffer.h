#ifndef _ICP_SOCKET_CIRCULAR_BUFFER_H_
#define _ICP_SOCKET_CIRCULAR_BUFFER_H_

#include <atomic>
#include <memory>
#include <optional>
#include <sys/mman.h>
#include <sys/uio.h>

#include "tl/expected.hpp"

namespace icp {
namespace socket {

class circular_buffer
{
    static constexpr size_t cache_line_size = 64;
    static constexpr size_t page_size = 4096;
    static constexpr size_t default_size = page_size * 16;

    uint8_t* m_buffer;
    size_t m_size;
    alignas(cache_line_size) std::atomic_uintptr_t m_read;
    alignas(cache_line_size) std::atomic_uintptr_t m_write;

    size_t read()  const { return m_read.load(std::memory_order_relaxed); }
    size_t write() const { return m_write.load(std::memory_order_relaxed); }

    circular_buffer(size_t initial_size);
    circular_buffer();
public:
    static circular_buffer server(size_t initial_size = default_size) { return (circular_buffer(initial_size)); }
    static circular_buffer client() { return (circular_buffer()); }
    ~circular_buffer();

    circular_buffer(const circular_buffer&) = delete;
    circular_buffer& operator=(const circular_buffer&&) = delete;

    circular_buffer& operator=(circular_buffer&& other);
    circular_buffer(circular_buffer&& other);

    size_t writable() const;
    size_t readable() const;
    bool   empty() const;
    bool   full() const;

    /***
     * Server side methods
     ***/

    /* TODO: for socket size support */
    //bool resize(size_t size);

    void* peek();
    size_t skip(size_t length);

    size_t read(void* ptr, size_t length);
    size_t read(iovec iov[], size_t iovcnt);

    size_t write(const void* ptr, size_t length);
    size_t write(const iovec iov[], size_t iovcnt);

    /***
     * Client side methods
     ***/

    /* read from a buffer in a separate process */
    tl::expected<size_t, int> read(pid_t pid, iovec iov[], size_t iovcnt);

    /* write to a buffer in a separate process */
    tl::expected<size_t, int> write(pid_t pid, const iovec iov[], size_t iovcnt);
};

}
}

#endif /* _ICP_SOCKET_CIRCULAR_BUFFER_H_ */
