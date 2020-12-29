#ifndef _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_
#define _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_

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

public:
    channels_hashtab()
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
        auto value = new ided_channel{
            id, io_channel_wrapper(channel, client_fd, server_fd)};
        return op_hashtab_insert(
            m_tab, reinterpret_cast<void*>(fd), reinterpret_cast<void*>(value));
    }

    inline ided_channel* find(int fd)
    {
        return reinterpret_cast<ided_channel*>(
            op_hashtab_find(m_tab, reinterpret_cast<void*>(fd)));
    };

    inline bool erase(int fd)
    {
        return op_hashtab_delete(m_tab, reinterpret_cast<void*>(fd));
    };

private:
    op_hashtab* m_tab;
};
} // namespace openperf::socket::api

#endif /* _OP_SOCKET_API_CHANNELS_HASHTAB_HPP_ */
