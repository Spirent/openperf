#include "zmq.h"

#include "packetio/workers/dpdk/callback.h"

namespace icp::packetio::dpdk {

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

static void dpdk_epoll_callback(int fd, void* arg)
{
    auto pollable = reinterpret_cast<callback*>(arg);
    assert(fd == pollable->event_fd());
    pollable->do_callback();
}

static int get_event_fd(const event_loop::event_notifier& notifier)
{
    return (std::visit(overloaded_visitor(
                           [](const void* socket) {
                               int fd = -1;
                               size_t fd_size = sizeof(fd);

                               return (zmq_getsockopt(const_cast<void*>(socket),
                                                      ZMQ_FD, &fd, &fd_size) == 0
                                       ? fd : -1);
                           },
                           [](const int fd) {
                               return (fd);
                           }),
                       notifier));
}

callback::callback(event_loop::generic_event_loop loop,
                   std::string_view name,
                   event_loop::event_notifier notifier,
                   event_loop::callback_function callback,
                   std::any arg)
    : m_loop(loop)
    , m_name(name)
    , m_notify(notifier)
    , m_callback(callback)
    , m_arg(arg)
{}

uint32_t callback::poll_id() const
{
    return (get_event_fd(m_notify));
}

std::string_view callback::name() const
{
    return (m_name);
}

event_loop::event_notifier callback::notifier() const
{
    return (m_notify);
}

void callback::do_callback()
{
    if (m_callback(m_loop, m_arg) == -1) {
        m_loop.del_callback(m_notify);
    }
}

int callback::event_fd() const
{
    return (get_event_fd(m_notify));
}

pollable_event<callback>::event_callback callback::event_callback_function() const
{
    return (dpdk_epoll_callback);
}

void* callback::event_callback_argument()
{
    return (this);
}

}
