#ifndef _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_
#define _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_

#include <memory>
#include <vector>

#include "packetio/generic_event_loop.h"
#include "packetio/internal_client.h"

namespace icp::packetio::dpdk::worker {

class event_loop_adapter
{
public:
    event_loop_adapter(void* context, workers::context ctx);
    ~event_loop_adapter();

    event_loop_adapter(event_loop_adapter&& other);
    event_loop_adapter& operator=(event_loop_adapter&& other);

    bool add_callback(std::string_view name,
                      event_loop::event_notifier notify,
                      event_loop::callback_function callback,
                      std::any arg) noexcept;
    void del_callback(event_loop::event_notifier notify) noexcept;

private:
    internal::api::client m_client;
    workers::context m_ctx;
    std::unordered_map<event_loop::event_notifier, std::string> m_tasks;
};

}

#endif /* _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_ */
