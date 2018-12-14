#ifndef _ICP_SOCKET_SPSC_QUEUE_H_
#define _ICP_SOCKET_SPSC_QUEUE_H_

#include <atomic>
#include <array>
#include <optional>

namespace icp {
namespace socket {

/**
 * This simple queue is a lock-free Single Producer / Single Consumer queue
 * based on a std::array.
 * Note: this queue must be a power-of-two size.
 */

template <typename T, std::size_t N>
class spsc_queue
{
    static constexpr size_t cache_line_size = 64;
    alignas(cache_line_size) std::atomic_size_t m_read;
    alignas(cache_line_size) std::atomic_size_t m_write;
    alignas(cache_line_size) std::array<T, N> m_data;

    size_t read()  { return m_read.load(std::memory_order_relaxed); }
    size_t write() { return m_write.load(std::memory_order_relaxed); }
    size_t mask(size_t value)  { return (value & (N - 1)); }

public:
    spsc_queue()
        : m_read(0)
        , m_write(0)
    {
        /* Make sure N is a non-zero power of two */
        static_assert(N != 0 && (N & (N - 1)) == 0);
    }

    bool   full()  { return (size() == N); }
    bool   empty() { return (read() == write()); }
    size_t size()  { return (write() - read()); }

    bool push(T&& value)
    {
        if (full()) {
            return (false);
        }
        std::atomic_thread_fence(std::memory_order_release);
        m_data[mask(write())] = std::forward<T>(value);
        m_write.fetch_add(1, std::memory_order_relaxed);
        return (true);
    }

    bool push(T& value)
    {
        return push(std::move(value));
    }

    std::optional<T> pop()
    {
        if (empty()) {
            return std::nullopt;
        }

        auto& value = m_data[mask(read())];
        m_read.fetch_add(1, std::memory_order_relaxed);
        return (std::make_optional<T>(value));
    }
};

}
}

#endif /* _ICP_SOCKET_SPSC_QUEUE_H_ */
