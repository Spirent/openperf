#include <algorithm>

#include "core/icp_log.h"
#include "packetio/workers/dpdk/event_loop_adapter.h"

namespace icp::packetio::dpdk::worker {

event_loop_adapter::~event_loop_adapter()
{
    for (auto& item : m_callbacks) {
        ICP_LOG(ICP_LOG_WARNING, "Leaking callback for fd = %d\n", item->event_fd());
    }
}

event_loop_adapter::event_loop_adapter(event_loop_adapter&& other)
    : m_callbacks(std::move(other.m_callbacks))
{}

event_loop_adapter& event_loop_adapter::operator=(event_loop_adapter&& other)
{
    if (this != &other) {
        m_callbacks = std::move(other.m_callbacks);
    }
    return (*this);
}

bool event_loop_adapter::add_callback(event_loop::generic_event_loop& loop,
                                      std::string_view name,
                                      event_loop::event_notifier notify,
                                      event_loop::callback_function cb,
                                      std::any arg) noexcept
{
    m_callbacks.push_back(std::make_unique<callback>(loop, name, notify, cb, arg));
    return (true);
}

void event_loop_adapter::del_callback(event_loop::event_notifier notify) noexcept
{
    m_callbacks.erase(std::remove_if(begin(m_callbacks), end(m_callbacks),
                                     [&](const callback_container::value_type& item) {
                                         return (item->notifier() == notify);
                                     }));
}

}
