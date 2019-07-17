#ifndef _ICP_PACKETIO_DPDK_WORKER_CONTROLLER_H_
#define _ICP_PACKETIO_DPDK_WORKER_CONTROLLER_H_

#include <memory>
#include <unordered_map>

#include "core/icp_uuid.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/generic_driver.h"
#include "packetio/generic_event_loop.h"
#include "packetio/generic_workers.h"
#include "packetio/workers/dpdk/callback.h"
#include "packetio/workers/dpdk/worker_api.h"

namespace icp::core {
class event_loop;
}

namespace icp::packetio::dpdk {

class worker_controller
{
public:
    worker_controller(void* context, icp::core::event_loop& loop,
                      driver::generic_driver& driver);
    ~worker_controller();

    /* controller is movable */
    worker_controller& operator=(worker_controller&& other);
    worker_controller(worker_controller&& other);

    /* controller is non-copyable */
    worker_controller(const worker_controller&) = delete;
    worker_controller& operator=(const worker_controller&&) = delete;

    workers::transmit_function get_transmit_function(std::string_view port_id) const;

    void add_interface(std::string_view port_id, std::any interface);
    void del_interface(std::string_view port_id, std::any interface);

    tl::expected<std::string, int> add_task(workers::context ctx,
                                            std::string_view name,
                                            event_loop::event_notifier notify,
                                            event_loop::event_handler on_event,
                                            std::optional<event_loop::delete_handler> on_delete,
                                            std::any arg);
    void del_task(std::string_view);

    using task_map = std::unordered_map<core::uuid, callback>;

private:
    void* m_context;
    driver::generic_driver& m_driver;
    std::unique_ptr<worker::client> m_workers;
    std::unique_ptr<worker::fib> m_fib;
    std::unique_ptr<worker::recycler> m_recycler;
    task_map m_tasks;
};

}

#endif /* _ICP_PACKETIO_DPDK_WORKER_CONTROLLER_H_ */
