#ifndef _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_
#define _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "packetio/generic_event_loop.h"
#include "packetio/workers/dpdk/callback.h"
#include "packetio/workers/dpdk/worker_api.h"

namespace icp::packetio::dpdk::worker {

class epoll_poller;

class event_loop_adapter
{
public:
    event_loop_adapter() = default;
    ~event_loop_adapter() = default;

    event_loop_adapter(event_loop_adapter&& other);
    event_loop_adapter& operator=(event_loop_adapter&& other);

    bool update_poller(epoll_poller& poller);

    bool add_callback(std::string_view name,
                      event_loop::event_notifier notify,
                      event_loop::event_handler on_event,
                      std::optional<event_loop::delete_handler> on_delete,
                      std::any arg) noexcept;
    void del_callback(event_loop::event_notifier notify) noexcept;

private:
    using callback_ptr = std::unique_ptr<callback>;

    std::vector<callback_ptr> m_additions;  /* new callbacks to run */
    std::vector<callback_ptr> m_deletions;  /* existing callbacks to delete */
    std::vector<callback_ptr> m_runnables;  /* active callbacks */
};

}

#endif /* _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_ */
