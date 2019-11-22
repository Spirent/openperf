#ifndef _OP_SOCKET_CIRCULAR_BUFFER_PRODUCER_HPP_
#define _OP_SOCKET_CIRCULAR_BUFFER_PRODUCER_HPP_

#include <atomic>
#include <sys/uio.h>

namespace openperf {
namespace socket {

template <typename Derived> class circular_buffer_producer
{
    /*
     * Align the 'writable' size to this value to try to limit
     * inefficient writes to our buffer.
     */
    static const size_t writable_alignment = 0x1F;

    Derived& derived();
    const Derived& derived() const;

    uint8_t* base() const;
    size_t len() const;
    std::atomic_size_t& read_idx();
    const std::atomic_size_t& read_idx() const;
    std::atomic_size_t& write_idx();
    const std::atomic_size_t& write_idx() const;

    size_t load_read() const;
    size_t load_write() const;
    size_t mask(size_t idx) const;

    void store_write(size_t idx);

public:
    size_t writable() const;
    size_t write(const void* ptr, size_t length);
    size_t write(const iovec iov[], size_t iovcnt);

    template <typename NotifyFunction>
    size_t write_and_notify(const void* ptr, size_t length,
                            NotifyFunction&& notify);
};

} // namespace socket
} // namespace openperf

#endif /* _OP_SOCKET_CIRCULAR_BUFFER_PRODUCER_HPP_ */
