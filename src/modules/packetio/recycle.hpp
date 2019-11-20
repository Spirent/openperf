#ifndef _OP_PACKETIO_RECYCLE_HPP_
#define _OP_PACKETIO_RECYCLE_HPP_

#include <atomic>
#include <bitset>
#include <functional>
#include <map>

namespace openperf::packetio::recycle {

/**
 * The depot contains all the logic and machinery necessary to implement
 * a Quiescent State Based Reclamation mechanism for safely deleting data
 * shared with a bunch of reader threads.
 **/
template <int NumReaders>
class depot {
    using gc_callback = std::function<void()>;
    static constexpr size_t cache_line_size = 64;
    static constexpr size_t base_version = 1;
    static constexpr size_t idle_version = 0;

    std::atomic_size_t m_writer_version;
    std::map<size_t, gc_callback> m_callbacks;
    std::bitset<NumReaders> m_active_readers;

    /*
     * Every reader/thread gets their own cache line sized area for writing
     * their currently observed version of the data.  We don't want to trigger
     * any cache sharing here as we expect reader threads to update this
     * frequently.
     */
    struct alignas(cache_line_size) reader_state {
        std::atomic_size_t version;
    };
    alignas(cache_line_size) std::array<reader_state, NumReaders> m_reader_states;

public:
    depot();
    ~depot() = default;

    /**
     * Non-thread safe functions; for writer use only!
     **/
    void writer_add_reader(size_t reader_id);
    void writer_del_reader(size_t reader_id);

    void writer_add_gc_callback(const gc_callback& callback);
    void writer_add_gc_callback(gc_callback&& callback);

    void writer_process_gc_callbacks();

    /**
     * Thread safe reader functions
     **/
    void reader_checkpoint(size_t reader_id);
    void reader_idle(size_t reader_id);
};

/**
 * This guard object provides a RAII based mechanism to protect reader access
 * to data that will be deleted via a depot based callback.
 **/
template <int NumReaders>
class guard
{
    depot<NumReaders>& m_depot;
    size_t m_id;

public:
    guard(depot<NumReaders>& depot, size_t reader_id);
    ~guard();
};

}

#endif /* _OP_PACKETIO_RECYCLE_HPP_ */
