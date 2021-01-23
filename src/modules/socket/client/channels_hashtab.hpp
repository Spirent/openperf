#ifndef _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_
#define _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_

#include <atomic>
#include <shared_mutex>

#include "core/op_hashtab.h"
#include "socket/client/io_channel_wrapper.hpp"
#include "utils/singleton.hpp"

namespace openperf::socket::api {
class channels_hashtab : public utils::singleton<channels_hashtab>
{
    static constexpr int gc_threshold = 10;

    using io_channel_wrapper = openperf::socket::client::io_channel_wrapper;
    struct ided_channel
    {
        socket_id id;
        io_channel_wrapper channel;
    };

public:
    channels_hashtab()
        : m_gc_count(0)
    {
        m_tab = op_hashtab_allocate();
        auto descructor = [](void* value) -> void {
            auto channel = reinterpret_cast<ided_channel*>(value);
            delete channel;
        };
        op_hashtab_set_value_destructor(m_tab, descructor);
    };
    ~channels_hashtab() { op_hashtab_free(&m_tab); };

    inline bool insert(int fd,
                       const socket_id& id,
                       const io_channel_ptr& channel,
                       int client_fd,
                       int server_fd)
    {
        std::shared_lock<std::shared_mutex> m_guard(m_lock);
        auto value = new ided_channel{
            id, io_channel_wrapper(channel, client_fd, server_fd)};
        return op_hashtab_insert(
            m_tab, reinterpret_cast<void*>(fd), reinterpret_cast<void*>(value));
    }

    inline ided_channel* find(int fd)
    {
        std::shared_lock<std::shared_mutex> m_guard(m_lock);
        return reinterpret_cast<ided_channel*>(
            op_hashtab_find(m_tab, reinterpret_cast<void*>(fd)));
    };

    inline bool erase(int fd)
    {
        if (!close_channel(fd)) { return false; }

        if (m_gc_count > gc_threshold) { garbage_collect(); }
        return true;
    };

private:
    inline bool close_channel(int fd)
    {
        std::shared_lock<std::shared_mutex> m_guard(m_lock);
        auto idc = reinterpret_cast<ided_channel*>(
            op_hashtab_find(m_tab, reinterpret_cast<void*>(fd)));
        if (!idc) return false;

        // Mark the channel as deleted.
        // The channel object is only actually deleted when
        // the garbage_collect() function is called.
        op_hashtab_delete(m_tab, reinterpret_cast<void*>(fd));
        m_gc_count += 1;

        // Need to mark the channel deleted before calling close() function
        // to avoid getting overridden close() when using LD_PRELOAD.
        idc->channel.close();
        return true;
    }

    inline void garbage_collect()
    {
        // hashtab garbage collection requires exclusive lock
        std::lock_guard<std::shared_mutex> m_guard(m_lock);
        op_hashtab_garbage_collect(m_tab);
        m_gc_count = 0;
    }

    op_hashtab* m_tab;

    // This shared mutex is used only used to obtain exclusive access
    // when doing garbage collection.  All other hashtab operations
    // are lock free thread safe so they don't need exclusive lock.
    std::shared_mutex m_lock;
    std::atomic<int> m_gc_count; // count of items pending garbage collection
};
} // namespace openperf::socket::api

#endif /* _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_ */
