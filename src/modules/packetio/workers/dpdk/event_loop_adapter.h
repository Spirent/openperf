#ifndef _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_
#define _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_

#include <memory>
#include <vector>

#include "packetio/generic_event_loop.h"
#include "packetio/workers/dpdk/callback.h"

namespace icp::packetio::dpdk::worker {

class event_loop_adapter
{
public:
    event_loop_adapter() = default;
    ~event_loop_adapter();

    event_loop_adapter(event_loop_adapter&& other);
    event_loop_adapter& operator=(event_loop_adapter&& other);

    bool add_callback(event_loop::generic_event_loop& loop,
                      std::string_view name,
                      event_loop::event_notifier notify,
                      event_loop::callback_function callback,
                      std::any arg) noexcept;
    void del_callback(event_loop::event_notifier notify) noexcept;

    using callback_container = std::vector<std::unique_ptr<callback>>;

private:
    callback_container m_callbacks;
};

}

#endif /* _ICP_PACKETIO_DPDK_WORKER_EVENT_LOOP_ADAPTER_H_ */
