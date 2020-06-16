#ifndef _OP_PACKETIO_DPDK_WORKER_CONTROLLER_HPP_
#define _OP_PACKETIO_DPDK_WORKER_CONTROLLER_HPP_

#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

#include "core/op_uuid.hpp"
#include "packetio/generic_driver.hpp"
#include "packetio/generic_event_loop.hpp"
#include "packetio/generic_workers.hpp"
#include "packetio/workers/dpdk/callback.hpp"
#include "packetio/drivers/dpdk/port/filter.hpp"
#include "packetio/drivers/dpdk/port/packet_type_decoder.hpp"
#include "packetio/drivers/dpdk/port/rss_hasher.hpp"
#include "packetio/drivers/dpdk/port/signature_decoder.hpp"
#include "packetio/drivers/dpdk/port/signature_encoder.hpp"
#include "packetio/drivers/dpdk/port/timestamper.hpp"
#include "packetio/workers/dpdk/port_feature_controller.hpp"
#include "packetio/workers/dpdk/tx_scheduler.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"
#include "utils/hash_combine.hpp"

struct rte_eth_rxtx_callback;

namespace openperf::core {
class event_loop;
}

namespace openperf::packetio::dpdk {

class worker_controller
{
public:
    worker_controller(void* context,
                      openperf::core::event_loop& loop,
                      driver::generic_driver& driver);
    ~worker_controller();

    /* controller is movable */
    worker_controller& operator=(worker_controller&& other) noexcept;
    worker_controller(worker_controller&& other) noexcept;

    /* controller is non-copyable */
    worker_controller(const worker_controller&) = delete;
    worker_controller& operator=(const worker_controller&&) = delete;

    std::vector<unsigned> get_rx_worker_ids(
        std::optional<std::string_view> obj_id = std::nullopt) const;
    std::vector<unsigned> get_tx_worker_ids(
        std::optional<std::string_view> obj_id = std::nullopt) const;

    workers::transmit_function
    get_transmit_function(std::string_view port_id) const;

    void add_interface(std::string_view port_id,
                       const interface::generic_interface& interface);
    void del_interface(std::string_view port_id,
                       const interface::generic_interface& interface);

    tl::expected<void, int> add_sink(std::string_view src_id,
                                     packet::generic_sink sink);
    void del_sink(std::string_view src_id, packet::generic_sink sink);

    tl::expected<void, int> add_source(std::string_view dst_id,
                                       packet::generic_source source);
    void del_source(std::string_view dst_id,
                    const packet::generic_source& source);
    tl::expected<void, int>
    swap_source(std::string_view dst_id,
                const packet::generic_source& outgoing,
                packet::generic_source incoming,
                std::optional<workers::source_swap_function>&& swap_action);

    tl::expected<std::string, int>
    add_task(workers::context ctx,
             std::string_view name,
             event_loop::event_notifier notify,
             event_loop::event_handler&& on_event,
             std::optional<event_loop::delete_handler>&& on_delete,
             std::any&& arg);
    void del_task(std::string_view task_id);

    using load_map = std::unordered_map<unsigned, uint64_t>;
    using task_map = std::unordered_map<core::uuid, callback>;
    using worker_map = std::map<std::pair<uint16_t, uint16_t>, unsigned>;
    using txsched_ptr = std::unique_ptr<tx_scheduler>;

    /*
     * XXX: order matters! The DPDK callbacks have no method to
     * enforce explicit ordering. The order of types in the template
     * is the order that the callbacks will be added to a linked list
     * in the DPDK internals. So, if callbacks have dependencies, then
     * the dependent callback needs to be listed after the independent
     * callback.
     */
    using sink_feature_controller =
        sink_feature_controller<port::filter,
                                port::timestamper,
                                port::packet_type_decoder,
                                port::rss_hasher,
                                port::signature_decoder>;
    using source_feature_controller =
        source_feature_controller<port::signature_encoder>;

private:
    void* m_context;                              /* 0MQ context */
    driver::generic_driver& m_driver;             /* generic driver reference */
    std::unique_ptr<worker::client> m_workers;    /* worker command client */
    std::unique_ptr<worker::fib> m_fib;           /* rx distpatch table */
    std::unique_ptr<worker::tib> m_tib;           /* transmit source table */
    std::unique_ptr<worker::recycler> m_recycler; /* RCU based deleter */

    task_map m_tasks; /* map from task id --> task object */

    std::vector<txsched_ptr> m_tx_schedulers; /* TX queue schedulers */
    load_map m_tx_loads;     /* map from worker id --> worker load */
    worker_map m_tx_workers; /* map from (port id, queue id) --> worker id */

    sink_feature_controller m_sink_features;
    source_feature_controller m_source_features;
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_WORKER_CONTROLLER_HPP_ */
