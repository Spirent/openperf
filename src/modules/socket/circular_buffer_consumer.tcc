#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <numeric>
#include <optional>

#include "socket/dpdk/memcpy.h"
#include "socket/circular_buffer_consumer.h"

namespace openperf {
namespace socket {

template <typename Derived>
Derived& circular_buffer_consumer<Derived>::derived()
{
    return (static_cast<Derived&>(*this));
}

template <typename Derived>
const Derived& circular_buffer_consumer<Derived>::derived() const
{
    return (static_cast<const Derived&>(*this));
}

template <typename Derived>
std::atomic_size_t& circular_buffer_consumer<Derived>::read_idx()
{
    return (derived().consumer_read_idx());
}

template <typename Derived>
const std::atomic_size_t& circular_buffer_consumer<Derived>::read_idx() const
{
    return (derived().consumer_read_idx());
}

template <typename Derived>
std::atomic_size_t& circular_buffer_consumer<Derived>::write_idx()
{
    return (derived().consumer_write_idx());
}

template <typename Derived>
const std::atomic_size_t& circular_buffer_consumer<Derived>::write_idx() const
{
    return (derived().consumer_write_idx());
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::len() const
{
    return (derived().consumer_len());
}

template <typename Derived>
uint8_t* circular_buffer_consumer<Derived>::base() const
{
    return (derived().consumer_base());
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::mask(size_t idx) const
{
    auto size = len();
    return (idx < size ? idx : idx - size);
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::load_read() const
{
    return (read_idx().load(std::memory_order_acquire));
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::load_write() const
{
    return (write_idx().load(std::memory_order_acquire));
}

template <typename Derived>
void circular_buffer_consumer<Derived>::store_read(size_t idx)
{
    auto dlen = 2 * len();
    read_idx().store(idx < dlen ? idx : idx - dlen,
                     std::memory_order_release);
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::readable() const
{
    auto read_cursor = load_read();
    auto write_cursor = load_write();

    return (read_cursor <= write_cursor
            ? write_cursor - read_cursor
            : (2 * len()) + write_cursor - read_cursor);
}

template <typename Derived>
std::array<iovec, 2> circular_buffer_consumer<Derived>::peek() const
{
    auto to_read = readable();
    if (!to_read) {
        return (std::array<iovec, 2>{});
    }

    auto cursor = load_read();

    /* Split the read into two chunks to account for the wrap of the buffer */
    const size_t chunk1 = std::min(to_read, len() - mask(cursor));
    const size_t chunk2 = to_read - chunk1;

    return (std::array<iovec, 2>{
            iovec{
                .iov_base = base() + mask(cursor),
                .iov_len = chunk1,
            },
            iovec{
                .iov_base = base(),
                .iov_len = chunk2,
            },
    });
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::drop(size_t length)
{
    auto to_drop = std::min(readable(), length);
    if (!to_drop) return (0);

    store_read(load_read() + to_drop);
    return (to_drop);
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::pread(void* ptr, size_t length, size_t offset)
{
    auto can_read = readable();
    auto to_read = std::min(can_read > offset ? can_read - offset : 0, length);
    if (!to_read) return (0);

    auto cursor = load_read() + offset;

    /* Split the read into two chunks to account for the wrap of the buffer */
    const size_t chunk1 = std::min(to_read, len() - mask(cursor));
    const size_t chunk2 = to_read - chunk1;

    dpdk::memcpy(ptr, base() + mask(cursor), chunk1);
    dpdk::memcpy(reinterpret_cast<uint8_t*>(ptr) + chunk1, base(), chunk2);

    return (to_read);
}

template <typename Derived>
size_t circular_buffer_consumer<Derived>::read(void* ptr, size_t length)
{
    return (drop(pread(ptr, length, 0)));
}

template <typename Derived>
template <typename NotifyFunction>
size_t circular_buffer_consumer<Derived>::read_and_notify(void *ptr, size_t length,
                                                          NotifyFunction&& notify)
{
    const auto available = readable();
    static auto lowat = available - (len() >> 1);
    auto to_read = std::min(available, length);
    if (!to_read) return (0);

    const auto chunk1 = std::min(to_read, lowat);
    const auto chunk2 = to_read - chunk1;
    assert(chunk1 + chunk2 == to_read);

    auto readed = read(ptr, chunk1);
    std::forward<NotifyFunction>(notify)();
    readed += read(reinterpret_cast<uint8_t*>(ptr) + chunk1, chunk2);

    return (readed);
}

}
}
