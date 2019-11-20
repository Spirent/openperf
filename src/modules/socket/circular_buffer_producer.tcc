#include <algorithm>
#include <cerrno>
#include <cstring>
#include <numeric>

#include "socket/dpdk/memcpy.h"
#include "socket/circular_buffer_producer.h"

namespace openperf {
namespace socket {

template <typename Derived>
Derived& circular_buffer_producer<Derived>::derived()
{
    return (static_cast<Derived&>(*this));
}

template <typename Derived>
const Derived& circular_buffer_producer<Derived>::derived() const
{
    return (static_cast<const Derived&>(*this));
}

template <typename Derived>
std::atomic_size_t& circular_buffer_producer<Derived>::read_idx()
{
    return (derived().producer_read_idx());
}

template <typename Derived>
const std::atomic_size_t& circular_buffer_producer<Derived>::read_idx() const
{
    return (derived().producer_read_idx());
}

template <typename Derived>
std::atomic_size_t& circular_buffer_producer<Derived>::write_idx()
{
    return (derived().producer_write_idx());
}

template <typename Derived>
const std::atomic_size_t& circular_buffer_producer<Derived>::write_idx() const
{
    return (derived().producer_write_idx());
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::len() const
{
    return (derived().producer_len());
}

template <typename Derived>
uint8_t* circular_buffer_producer<Derived>::base() const
{
    return (derived().producer_base());
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::mask(size_t idx) const
{
    auto size = len();
    return (idx < size ? idx : idx - size);
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::load_read() const
{
    return (read_idx().load(std::memory_order_acquire));
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::load_write() const
{
    return (write_idx().load(std::memory_order_acquire));
}

template <typename Derived>
void circular_buffer_producer<Derived>::store_write(size_t idx)
{
    auto dlen = 2 * len();
    write_idx().store(idx < dlen ? idx : idx - dlen,
                      std::memory_order_release);
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::writable() const
{
    auto read_cursor = load_read();
    auto write_cursor = load_write();

    auto available = (read_cursor <= write_cursor
                      ? len() - write_cursor + read_cursor
                      : read_cursor - write_cursor - len());

    /* Try to limit unaligned writes to the buffer */
    return (available & ~(writable_alignment));
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::write(const void* ptr, size_t length)
{
    auto to_write = std::min(writable(), length);
    if (!to_write) return (0);

    auto cursor = load_write();

    /* Split the write into two chunks to account for the wrap of the buffer */
    const size_t chunk1 = std::min(to_write, len() - mask(cursor));
    const size_t chunk2 = to_write - chunk1;

    dpdk::memcpy(base() + mask(cursor), ptr, chunk1);
    dpdk::memcpy(base(), reinterpret_cast<const uint8_t*>(ptr) + chunk1, chunk2);

    store_write(cursor + to_write);
    return (to_write);
}

template <typename Derived>
size_t circular_buffer_producer<Derived>::write(const iovec iov[], size_t iovcnt)
{
    const auto to_write = std::min(writable(),
                                   std::accumulate(iov, iov + iovcnt, 0UL,
                                                   [](size_t x, const iovec& iov) {
                                                       return (x + iov.iov_len);
                                                   }));
    if (!to_write) return (0);

    auto cursor = load_write();

    const auto chunk1 = std::min(to_write, len() - mask(cursor));
    const auto chunk2 = to_write - chunk1;
    size_t iov_idx = 0, written1 = 0;

    /* Copy full iovs before the buffer wrap */
    while (written1 + iov[iov_idx].iov_len <= chunk1
           && iov_idx < iovcnt) {
        dpdk::memcpy(base() + mask(cursor) + written1, iov[iov_idx].iov_base, iov[iov_idx].iov_len);
        written1 += iov[iov_idx].iov_len;
        iov_idx++;
    }

    /* All writes before the wrap; return what was written */
    if (chunk1 == to_write) {
        store_write(cursor + written1);
        return (written1);
    }

    /*
     * Next write is across the end of the buffer.  Divide the vector into
     * two pieces: one before the wrap and one after.
     */
    const auto piece1 = chunk1 - written1;
    const auto piece2 = iov[iov_idx].iov_len - piece1;
    assert(piece1 + piece2 == iov[iov_idx].iov_len);

    /*
     * If we don't have enough room to write the wrapped portion into the
     * buffer, then skip writing any of it.  We don't want to copy a partial
     * iovec.
     */
    if (chunk2 < piece2) {
        store_write(cursor + written1);
        return(written1);
    }

    /* Copy the two pieces */
    dpdk::memcpy(base() + mask(cursor) + written1, iov[iov_idx].iov_base, piece1);
    written1 += piece1;
    dpdk::memcpy(base(), reinterpret_cast<const uint8_t*>(iov[iov_idx].iov_base) + piece1, piece2);
    auto written2 = piece2;
    iov_idx++;

    /* Now finish writing iovs to the front of the buffer */
    while (written2 + iov[iov_idx].iov_len <= chunk2
           && iov_idx < iovcnt) {
        dpdk::memcpy(base() + written2, iov[iov_idx].iov_base, iov[iov_idx].iov_len);
        written2 += iov[iov_idx].iov_len;
        iov_idx++;
    }

    store_write(cursor + written1 + written2);
    return (written1 + written2);
}

template <typename Derived>
template <typename NotifyFunction>
size_t circular_buffer_producer<Derived>::write_and_notify(const void* ptr, size_t length,
                                                           NotifyFunction&& notify)
{
    const auto available = writable();
    const auto hiwat = available - (len() >> 1);
    auto to_write = std::min(available, length);
    if (!to_write) return (0);

    const auto chunk1 = std::min(to_write, hiwat);
    const auto chunk2 = to_write - chunk1;
    assert(chunk1 + chunk2 == to_write);

    auto written = write(ptr, chunk1);
    std::forward<NotifyFunction>(notify)();
    written += write(reinterpret_cast<const uint8_t*>(ptr) + chunk1, chunk2);

    return (written);
}

}
}
