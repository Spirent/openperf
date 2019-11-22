#ifndef _OP_EVENT_LOOP_HPP_
#define _OP_EVENT_LOOP_HPP_

#include <cstddef>
#include <memory>
#include "core/op_event_loop.h"

namespace openperf {
namespace core {

class event_loop
{
public:
    event_loop()
        : m_loop(op_event_loop_allocate()){};
    ~event_loop() {}

    size_t count() { return op_event_loop_count(m_loop.get()); }
    void purge() { return op_event_loop_purge(m_loop.get()); }

    int run() { return op_event_loop_run_forever(m_loop.get()); }
    int run(int timeout)
    {
        return op_event_loop_run_timeout(m_loop.get(), timeout);
    }
    void exit() { return op_event_loop_exit(m_loop.get()); }

    // Add callbacks
    int add(int fd, const struct op_event_callbacks* callbacks, void* arg)
    {
        return op_event_loop_add_fd(m_loop.get(), fd, callbacks, arg);
    }
    int add(void* socket, const struct op_event_callbacks* callbacks, void* arg)
    {
        return op_event_loop_add_zmq(m_loop.get(), socket, callbacks, arg);
    }
    int add(uint64_t timeout, const struct op_event_callbacks* callbacks,
            void* arg, uint32_t* timeout_id = nullptr)
    {
        return op_event_loop_add_timer_ided(m_loop.get(), timeout, callbacks,
                                            arg, timeout_id);
    }

    // Update callbacks
    int update(int fd, const struct op_event_callbacks* callbacks)
    {
        return op_event_loop_update_fd(m_loop.get(), fd, callbacks);
    }
    int update(void* socket, const struct op_event_callbacks* callbacks)
    {
        return op_event_loop_update_zmq(m_loop.get(), socket, callbacks);
    }
    int update(uint32_t timeout_id, const struct op_event_callbacks* callbacks)
    {
        return op_event_loop_update_timer_cb(m_loop.get(), timeout_id,
                                             callbacks);
    }
    int update(uint32_t timeout_id, uint64_t timeout)
    {
        return op_event_loop_update_timer_to(m_loop.get(), timeout_id, timeout);
    }

    // Disable callbacks
    int disable(int fd) { return op_event_loop_disable_fd(m_loop.get(), fd); }
    int disable(void* socket)
    {
        return op_event_loop_disable_zmq(m_loop.get(), socket);
    }
    int disable(uint32_t timeout_id)
    {
        return op_event_loop_disable_timer(m_loop.get(), timeout_id);
    }

    // Delete callbacks
    int del(int fd) { return op_event_loop_del_fd(m_loop.get(), fd); }
    int del(void* socket)
    {
        return op_event_loop_del_zmq(m_loop.get(), socket);
    }
    int del(uint32_t timeout_id)
    {
        return op_event_loop_del_timer(m_loop.get(), timeout_id);
    }

private:
    struct loop_deleter
    {
        void operator()(struct op_event_loop* loop) const
        {
            op_event_loop_free(&loop);
        }
    };

    std::unique_ptr<struct op_event_loop, loop_deleter> m_loop;
};

} // namespace core
} // namespace openperf
#endif /* _OP_EVENT_LOOP_HPP_ */
