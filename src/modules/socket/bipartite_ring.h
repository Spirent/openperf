#ifndef _ICP_SOCKET_BIPARTITE_RING_H_
#define _ICP_SOCKET_BIPARTITE_RING_H_

#include <algorithm>
#include <array>
#include <atomic>
#include <optional>

namespace icp {
namespace socket {

/*
 * This struct provides a single consumer, single producer bipartite ring.
 * By bipartite, we mean there are two sections of the ring.  A section
 * consisting of items produced by a 'server' ready for a 'client' to use
 * and a section of items used by the 'client' and ready for the 'server'
 * to do something with.
 *
 * The names for these functions needs work...
 */
template <typename T, std::size_t N>
class bipartite_ring
{
    static constexpr size_t cache_line_size = 64;
    alignas(cache_line_size) struct server_indexes {
        std::atomic_size_t head;
        std::atomic_size_t tail;
    } m_server;
    alignas(cache_line_size) struct client_indexes {
        std::atomic_size_t cursor;
        size_t index;
    } m_client;
    alignas(cache_line_size) std::array<T, N> m_data;

    size_t head() const { return (m_server.head.load(std::memory_order_relaxed)); }
    size_t tail() const { return (m_server.tail.load(std::memory_order_relaxed)); }
    size_t cursor() const { return (m_client.cursor.load(std::memory_order_relaxed)); }
    size_t index() const { return (m_client.index); }

    size_t mask(size_t value) const { return (value & (N - 1)); }

    void do_enqueue(T&& value)
    {
        m_data[mask(m_server.head.fetch_add(1, std::memory_order_relaxed))] = std::forward<T>(value);
    }

    T& do_dequeue()
    {
        return (m_data[mask(m_server.tail.fetch_add(1, std::memory_order_relaxed))]);
    }

    bipartite_ring(bool do_init)
    {
        /* N must be a non-zero power of two */
        static_assert(N != 0 && (N & (N - 1)) == 0);
        if (do_init) {
            m_server.head = ATOMIC_VAR_INIT(0);
            m_server.tail = ATOMIC_VAR_INIT(0);
            m_client.cursor = ATOMIC_VAR_INIT(0);
            m_client.index = 0;
        }
    }

public:
    static bipartite_ring server() { return (bipartite_ring(true)); };
    static bipartite_ring client() { return (bipartite_ring(false)); };

    /***
     * Server side functions
     ***/
    size_t size() const { return (head() - tail()); }
    bool   full() const { return (size() == N); }
    bool   empty() const { return (head() == tail()); }
    size_t capacity() const { return (N - size()); }

    bool enqueue(T&& value)
    {
        if (full()) {
            return (false);
        }
        do_enqueue(std::forward<T>(value));
        return (true);
    }

    bool enqueue(T& value)
    {
        return (enqueue(std::move(value)));
    }

    size_t enqueue(T items[], size_t nb_items)
    {
        auto to_enqueue = std::min(capacity(), nb_items);
        for (size_t idx = 0; idx < to_enqueue; idx++) {
            do_enqueue(std::move(items[idx]));
        }
        return (to_enqueue);
    }

    size_t ready() const { return (cursor() - tail()); }

    std::optional<T> dequeue()
    {
        return (ready()
                ? std::make_optional<T>(do_dequeue())
                : std::nullopt);
    }

    size_t dequeue(T items[], size_t max_items)
    {
        auto to_dequeue = std::min(ready(), max_items);
        for (size_t idx = 0; idx < to_dequeue; idx++) {
            items[idx] = do_dequeue();
        }
        return (to_dequeue);
    }

    /***
     * Client side functions
     ***/
    size_t available() const { return (head() - index()); }
    bool idle() const { return (index() == cursor()); }

    T* const unpack()
    {
        if (!available()) {
            return (nullptr);
        }

        return (&m_data[mask(m_client.index++)]);
    }

    size_t unpack(T* items[], size_t max_items)
    {
        auto nb_items = std::min(available(), max_items);
        for (size_t idx = 0; idx < nb_items; idx++) {
            items[idx] = &m_data[mask(m_client.index++)];
        }
        return (nb_items);
    }

    void repack() {
        m_client.cursor.store(m_client.index, std::memory_order_relaxed);
    }
};

}
}
#endif /* _ICP_SOCKET_BIPARTITE_RING_H_ */
