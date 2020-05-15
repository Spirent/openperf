#ifndef _OP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_HPP_
#define _OP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_HPP_

#include <memory>
#include <unordered_map>
#include <vector>

#include "packetio/generic_event_loop.hpp"
#include "packetio/workers/dpdk/callback.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"

namespace openperf::packetio::dpdk::worker {

class epoll_poller;

class event_loop_adapter
{
public:
    event_loop_adapter() = default;
    ~event_loop_adapter() = default;

    event_loop_adapter(event_loop_adapter&& other) noexcept;
    event_loop_adapter& operator=(event_loop_adapter&& other) noexcept;

    bool update_poller(epoll_poller& poller);

    bool add_callback(std::string_view name,
                      event_loop::event_notifier notify,
                      event_loop::event_handler&& on_event,
                      std::optional<event_loop::delete_handler>&& on_delete,
                      std::any&& arg) noexcept;
    void del_callback(event_loop::event_notifier notify) noexcept;

private:
    using callback_ptr = std::unique_ptr<callback>;

    std::vector<callback_ptr> m_additions; /* new callbacks to run */
    std::vector<callback_ptr> m_deletions; /* existing callbacks to delete */
    std::vector<callback_ptr> m_runnables; /* active callbacks */
};

} // namespace openperf::packetio::dpdk::worker

#endif /* _OP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_HPP_ */
