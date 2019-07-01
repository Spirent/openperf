#include <algorithm>

#include "core/icp_log.h"
#include "packetio/internal_client.h"
#include "packetio/workers/dpdk/event_loop_adapter.h"

namespace icp::packetio::dpdk::worker {

event_loop_adapter::event_loop_adapter(void* context, workers::context ctx)
    : m_client(context)
    , m_ctx(ctx)
{}

event_loop_adapter::~event_loop_adapter()
{
    for (auto& [notify, id] : m_tasks) {
        m_client.del_task(id);
    }
}

event_loop_adapter::event_loop_adapter(event_loop_adapter&& other)
    : m_client(std::move(other.m_client))
    , m_ctx(other.m_ctx)
    , m_tasks(std::move(other.m_tasks))
{}

event_loop_adapter& event_loop_adapter::operator=(event_loop_adapter&& other)
{
    if (this != &other) {
        m_client = std::move(other.m_client);
        m_ctx = other.m_ctx;
        m_tasks = std::move(other.m_tasks);
    }
    return (*this);
}

bool event_loop_adapter::add_callback(std::string_view name,
                                      event_loop::event_notifier notify,
                                      event_loop::callback_function cb,
                                      std::any arg) noexcept
{
    auto result = m_client.add_task(m_ctx, name, notify, cb, arg);
    if (!result) {
        ICP_LOG(ICP_LOG_ERROR, "Could not register task named %.*s: %s\n",
                static_cast<int>(name.length()), name.data(), strerror(result.error()));
        return (false);
    }
    m_tasks.insert({notify, std::move(*result)});
    return (true);
}

void event_loop_adapter::del_callback(event_loop::event_notifier notify) noexcept
{
    /* Find the id for the given notifier */
    if (auto item = m_tasks.find(notify); item != m_tasks.end()) {
        m_client.del_task(item->second);
        m_tasks.erase(item);
    }
}

}
