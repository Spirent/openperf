#include <unistd.h>

#include "zmq.h"

#include "packetio/workers/dpdk/callback.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packetio::dpdk {

static int get_event_fd(const event_loop::event_notifier& notifier)
{
    return (std::visit(utils::overloaded_visitor(
                           [](const void* socket) {
                               int fd = -1;
                               size_t fd_size = sizeof(fd);

                               return (zmq_getsockopt(const_cast<void*>(socket),
                                                      ZMQ_FD, &fd, &fd_size)
                                               == 0
                                           ? fd
                                           : -1);
                           },
                           [](const int fd) { return (fd); }),
                       notifier));
}

static void close_event_fd(event_loop::event_notifier& notifier)
{
    return (std::visit(
        utils::overloaded_visitor([](void* socket) { zmq_close(socket); },
                                  [](int fd) { close(fd); }),
        notifier));
}

callback::callback(std::string_view name, event_loop::event_notifier notifier,
                   event_loop::event_handler on_event,
                   std::optional<event_loop::delete_handler> on_delete,
                   std::any arg)
    : m_name(name)
    , m_notify(std::move(notifier))
    , m_on_event(std::move(on_event))
    , m_on_delete(std::move(on_delete))
    , m_arg(std::move(arg))
{}

callback::~callback()
{
    if (m_on_delete) { m_on_delete.value()(m_arg); }
    close_event_fd(m_notify);
}

std::string_view callback::name() const { return (m_name); }

event_loop::event_notifier callback::notifier() const { return (m_notify); }

int callback::event_fd() const { return (get_event_fd(m_notify)); }

void callback::run_callback(event_loop::generic_event_loop& loop)
{
    auto error = m_on_event(loop, m_arg);
    if (error) { loop.del_callback(m_notify); }
}

} // namespace openperf::packetio::dpdk
