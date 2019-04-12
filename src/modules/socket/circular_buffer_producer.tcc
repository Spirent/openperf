#include <algorithm>
#include <cerrno>
#include <cstring>
#include <numeric>

#include "socket/dpdk_memcpy.h"
#include "socket/circular_buffer_producer.h"

namespace icp {
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

    return (read_cursor <= write_cursor
            ? len() - write_cursor + read_cursor
            : read_cursor - write_cursor - len());
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
template <typename NotifyFunction>
size_t circular_buffer_producer<Derived>::write_and_notify(const void* ptr, size_t length,
                                                           NotifyFunction&& notify)
{
    const auto available = writable();
    static auto hiwat = available - (len() >> 1);
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
