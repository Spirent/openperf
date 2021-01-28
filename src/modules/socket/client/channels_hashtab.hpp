#ifndef _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_
#define _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_

#include <shared_mutex>

#include "core/op_hashtab.h"
#include "socket/client/io_channel_wrapper.hpp"
#include "utils/singleton.hpp"

namespace openperf::socket::api {
class channels_hashtab : public utils::singleton<channels_hashtab>
{
    using io_channel_wrapper = openperf::socket::client::io_channel_wrapper;
    struct ided_channel
    {
        socket_id id;
        io_channel_wrapper channel;
    };
    using ided_channel_ptr = std::shared_ptr<ided_channel>;

public:
    channels_hashtab()
    {
        m_tab = op_hashtab_allocate();
        auto descructor = [](void* value) -> void {
            auto channel = reinterpret_cast<ided_channel_ptr*>(value);
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
        auto value = new ided_channel_ptr(new ided_channel{
            id, io_channel_wrapper(channel, client_fd, server_fd)});
        return op_hashtab_insert(
            m_tab, reinterpret_cast<void*>(fd), reinterpret_cast<void*>(value));
    }

    inline ided_channel_ptr find(int fd)
    {
        std::shared_lock<std::shared_mutex> m_guard(m_lock);
        auto ptr = reinterpret_cast<ided_channel_ptr*>(
            op_hashtab_find(m_tab, reinterpret_cast<void*>(fd)));
        if (!ptr) return {};
        return *ptr;
    };

    inline bool erase(int fd)
    {
        if (!close_channel(fd)) { return false; }

        // Perform garbage collection to actually delete the channel object
        garbage_collect();
        return true;
    };

private:
    inline bool close_channel(int fd)
    {
        auto ptr = find(fd);
        if (!ptr) return false;

        // Mark the channel deleted
        op_hashtab_delete(m_tab, reinterpret_cast<void*>(fd));

        // Close the channel eventfds
        //
        // When using LD_PRELOAD all close() calls will get the overridden
        // close function so don't want those calls to find the eventfd in
        // the hashtab or to see a locked mutex.  There is currently no
        // recursive_shared_mutex in the standard library.
        //
        ptr->channel.close();
        return true;
    }

    inline void garbage_collect()
    {
        // hashtab garbage collection requires exclusive lock
        std::lock_guard<std::shared_mutex> m_guard(m_lock);
        op_hashtab_garbage_collect(m_tab);
    }

    op_hashtab* m_tab;

    // This shared mutex is used only used to obtain exclusive access
    // when doing garbage collection.  All other hashtab operations
    // are lock free thread safe so they don't need exclusive lock.
    std::shared_mutex m_lock;
};
} // namespace openperf::socket::api

#endif /* _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_ */
