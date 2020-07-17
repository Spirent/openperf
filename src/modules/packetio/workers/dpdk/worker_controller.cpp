#include <algorithm>
#include <chrono>
#include <vector>

#include "core/op_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/physical_port.hpp"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "packetio/stack/dpdk/net_interface.hpp"
#include "packetio/workers/dpdk/event_loop_adapter.hpp"
#include "packetio/workers/dpdk/port_feature_controller.tcc"
#include "packetio/workers/dpdk/tx_source.hpp"
#include "packetio/workers/dpdk/worker_tx_functions.hpp"
#include "packetio/workers/dpdk/worker_queues.hpp"
#include "packetio/workers/dpdk/worker_controller.hpp"

namespace openperf::packetio::dpdk {

using mac_address = libpacket::type::mac_address;

static void launch_workers(void* context,
                           worker::recycler* recycler,
                           const worker::fib* fib)
{
    /* Launch work threads on all of our available worker cores */
    static std::string_view sync_endpoint = "inproc://dpdk_worker_sync";
    auto sync = op_task_sync_socket(context, sync_endpoint.data());
    struct worker::main_args args = {
        .context = context,
        .endpoint = sync_endpoint.data(),
        .recycler = recycler,
        .fib = fib,
    };

    /* Launch a worker on every available core */
    if (rte_eal_mp_remote_launch(worker::main, &args, SKIP_MASTER) != 0) {
        throw std::runtime_error("DPDK worker core is busy!");
    }

    /*
     * Wait until all workers have pinged us back.  If we send out the
     * configuration before all of the workers are ready, they could miss it.
     */
    op_task_sync_block(&sync, rte_lcore_count() - 1);
}

static std::vector<worker::descriptor> to_worker_descriptors(
    const std::vector<queue::descriptor>& descriptors,
    const std::vector<worker_controller::txsched_ptr>& tx_schedulers,
    const worker_controller::worker_map& tx_workers,
    const worker::port_queues& queues)
{
    std::vector<worker::descriptor> worker_items;

    for (auto& d : descriptors) {
        assert(d.mode != queue::queue_mode::NONE);
        switch (d.mode) {
        case queue::queue_mode::RX:
            worker_items.emplace_back(d.worker_id,
                                      queues[d.port_id].rx(d.queue_id));
            break;
        case queue::queue_mode::TX:
            worker_items.emplace_back(d.worker_id,
                                      queues[d.port_id].tx(d.queue_id));
            break;
        case queue::queue_mode::RXTX:
            worker_items.emplace_back(d.worker_id,
                                      queues[d.port_id].rx(d.queue_id));
            worker_items.emplace_back(d.worker_id,
                                      queues[d.port_id].tx(d.queue_id));
            break;
        default:
            break;
        }
    }

    for (auto& sched_ptr : tx_schedulers) {
        auto idx = tx_workers.at(
            std::make_pair(sched_ptr->port_id(), sched_ptr->queue_id()));
        worker_items.emplace_back(idx, sched_ptr.get());
    }

    return (worker_items);
}

static std::vector<model::port_info> get_port_info()
{
    uint16_t port_id = 0;
    std::vector<model::port_info> port_info;
    RTE_ETH_FOREACH_DEV (port_id) {
        port_info.emplace_back(model::port_info(port_id));
    }

    return (port_info);
}

static std::vector<queue::descriptor>
get_queue_descriptors(std::vector<model::port_info>& port_info)
{
    auto q_descriptors = topology::queue_distribute(port_info);

    /*
     * XXX: If we have only one worker thread, then everything should use the
     * direct transmit functions.  Hence, we don't need any tx queues.
     */
    if (rte_lcore_count() <= 2) {
        /* Filter out all tx queues; we don't need them */
        q_descriptors.erase(
            std::remove_if(begin(q_descriptors),
                           end(q_descriptors),
                           [](const queue::descriptor& d) {
                               return (d.mode == queue::queue_mode::TX);
                           }),
            end(q_descriptors));

        /* Change all RXTX queues to RX only */
        for (auto& d : q_descriptors) {
            if (d.mode == queue::queue_mode::RXTX) {
                d.mode = queue::queue_mode::RX;
            }
        }
    }

    return (q_descriptors);
}

static unsigned num_workers() { return (rte_lcore_count() - 1); }

static int handle_recycler_timeout(const op_event_data*, void* arg)
{
    auto recycler = reinterpret_cast<worker::recycler*>(arg);
    recycler->writer_process_gc_callbacks();
    return (0);
}

static void setup_recycler_callback(openperf::core::event_loop& loop,
                                    worker::recycler* recycler)
{
    using namespace std::chrono_literals;
    std::chrono::duration<uint64_t, std::nano> timeout = 5s;

    struct op_event_callbacks callbacks = {.on_timeout =
                                               handle_recycler_timeout};

    loop.add(timeout.count(), &callbacks, recycler);
}

template <typename T>
T& get_unique_port_object(std::vector<std::unique_ptr<T>>& things,
                          uint16_t port_id)
{
    auto item =
        std::find_if(std::begin(things), std::end(things), [&](auto& thing) {
            return (port_id == thing->port_id());
        });

    /* Thing should always exist */
    assert(item != things.end());

    return (*(item->get()));
}

static void maybe_enable_rxq_tag_detection(const port::filter& filter)
{
    if (filter.type() == port::filter_type::flow) {
        auto& queues = worker::port_queues::instance();
        auto& container = queues[filter.port_id()];
        for (uint16_t i = 0; i < container.rx_queues(); i++) {
            auto rxq = container.rx(i);
            OP_LOG(OP_LOG_DEBUG,
                   "Enabling hardware flow tag detection on RX queue %u:%u\n",
                   rxq->port_id(),
                   rxq->queue_id());
            rxq->flags(rxq->flags() | rx_feature_flags::hardware_tags);
        }
    }
}

static void maybe_disable_rxq_tag_detection(const port::filter& filter)
{
    if (filter.type() == port::filter_type::flow) {
        auto& queues = worker::port_queues::instance();
        auto& container = queues[filter.port_id()];
        for (uint16_t i = 0; i < container.rx_queues(); i++) {
            auto rxq = container.rx(i);
            OP_LOG(OP_LOG_DEBUG,
                   "Disabling hardware flow tag detection on RX queue %u:%u\n",
                   rxq->port_id(),
                   rxq->queue_id());
            rxq->flags(rxq->flags() & ~rx_feature_flags::hardware_tags);
        }
    }
}

static void maybe_update_rxq_lro_mode(const model::port_info& info)
{
    if (info.rx_offloads() & DEV_RX_OFFLOAD_TCP_LRO) {
        auto& queues = worker::port_queues::instance();
        auto& container = queues[info.id()];
        for (uint16_t i = 0; i < container.rx_queues(); i++) {
            auto rxq = container.rx(i);
            OP_LOG(OP_LOG_DEBUG,
                   "Disabling software LRO on RX queue %u:%u. "
                   "Hardware support detected\n",
                   rxq->port_id(),
                   rxq->queue_id());
            rxq->flags(rxq->flags() | rx_feature_flags::hardware_lro);
        }
    }
}

static void handle_tx_sink(net_interface& interface,
                           struct rte_mbuf** pkts,
                           uint16_t pkts_len,
                           void* cbdata)
{
    auto fib = reinterpret_cast<worker::fib*>(cbdata);
    auto sinks = fib->find_interface_sinks(interface.port_index(),
                                           interface.data()->hwaddr,
                                           worker::fib::direction::TX);
    if (sinks) {
        for (auto& sink : *sinks) {
            sink.push(reinterpret_cast<packet::packet_buffer**>(pkts),
                      pkts_len);
        }
    }
}

worker_controller::worker_controller(void* context,
                                     openperf::core::event_loop& loop,
                                     driver::generic_driver& driver)
    : m_context(context)
    , m_driver(driver)
    , m_workers(std::make_unique<worker::client>(context))
    , m_fib(std::make_unique<worker::fib>())
    , m_tib(std::make_unique<worker::tib>())
    , m_recycler(std::make_unique<worker::recycler>())
{
    launch_workers(context, m_recycler.get(), m_fib.get());

    /*
     * For each worker we need to...
     * 1. Update QSBR based memory reclamation with worker info
     * 2. Initialize the transmit worker load map.
     */
    unsigned lcore_id;
    RTE_LCORE_FOREACH_SLAVE (lcore_id) {
        m_recycler->writer_add_reader(lcore_id);
    }

    /* And create the periodic callback */
    setup_recycler_callback(loop, m_recycler.get());

    /* We need port information to setup our workers, so get it */
    auto port_info = get_port_info();

    /* Construct our necessary transmit schedulers and metadata */
    for (auto& d : topology::queue_distribute(port_info)) {
        if (d.mode == queue::queue_mode::RX) continue;
        m_tx_schedulers.push_back(
            std::make_unique<tx_scheduler>(*m_tib, d.port_id, d.queue_id));
        m_tx_loads.emplace(d.worker_id, 0);
        m_tx_workers.emplace(std::make_pair(d.port_id, d.queue_id),
                             d.worker_id);
    }

    /* Construct our necessary port queues */
    auto q_descriptors = get_queue_descriptors(port_info);
    auto& queues = worker::port_queues::instance();
    queues.setup(q_descriptors);

    /* Update queues to take advantage of port capabilities */
    /* XXX: queues must be setup first! */
    const auto& filters = m_sink_features.get<port::filter>();
    std::for_each(
        std::begin(filters), std::end(filters), [](const auto& filter) {
            maybe_enable_rxq_tag_detection(*filter);
        });
    std::for_each(std::begin(port_info),
                  std::end(port_info),
                  [](const auto& info) { maybe_update_rxq_lro_mode(info); });

    /* Distribute queues and schedulers to workers */
    m_workers->add_descriptors(to_worker_descriptors(
        q_descriptors, m_tx_schedulers, m_tx_workers, queues));

    /* And start them */
    m_driver.start_all_ports();
    m_workers->start(m_context, num_workers());
}

worker_controller::~worker_controller()
{
    if (!m_workers) return;

    m_workers->stop(m_context, num_workers());
    m_driver.stop_all_ports();
    m_recycler->writer_process_gc_callbacks();
    rte_eal_mp_wait_lcore();
    worker::port_queues::instance().unset();
}

worker_controller::worker_controller(worker_controller&& other) noexcept
    : m_context(other.m_context)
    , m_driver(other.m_driver)
    , m_workers(std::move(other.m_workers))
    , m_fib(std::move(other.m_fib))
    , m_tib(std::move(other.m_tib))
    , m_recycler(std::move(other.m_recycler))
    , m_tasks(std::move(other.m_tasks))
    , m_tx_schedulers(std::move(other.m_tx_schedulers))
    , m_tx_loads(std::move(other.m_tx_loads))
    , m_tx_workers(std::move(other.m_tx_workers))
    , m_sink_features(std::move(other.m_sink_features))
    , m_source_features(std::move(other.m_source_features))
{}

worker_controller&
worker_controller::operator=(worker_controller&& other) noexcept
{
    if (this != &other) {
        m_context = other.m_context;
        m_driver = std::move(other.m_driver);
        m_workers = std::move(other.m_workers);
        m_fib = std::move(other.m_fib);
        m_tib = std::move(other.m_tib);
        m_recycler = std::move(other.m_recycler);
        m_tasks = std::move(other.m_tasks);
        m_tx_schedulers = std::move(other.m_tx_schedulers);
        m_tx_loads = std::move(other.m_tx_loads);
        m_tx_workers = std::move(other.m_tx_workers);
        m_sink_features = std::move(other.m_sink_features);
        m_source_features = std::move(other.m_source_features);
    }
    return (*this);
}

static std::vector<unsigned>
get_queue_worker_ids(queue::queue_mode type,
                     std::optional<int> port_id = std::nullopt)
{
    auto port_info = get_port_info();
    auto q_descriptors = topology::queue_distribute(port_info);

    /* If caller gave us a port, filter out all non-matching descriptors */
    if (port_id) {
        q_descriptors.erase(std::remove_if(std::begin(q_descriptors),
                                           std::end(q_descriptors),
                                           [&](const auto& d) {
                                               return (d.port_id != *port_id);
                                           }),
                            std::end(q_descriptors));
    }

    /*
     * Our queue descriptors vector contains all port queues along with
     * their assigned workers.  Go through the data finding all queues of
     * the correct type and add the associated worker to our vector of workers.
     */
    std::vector<unsigned> workers;
    std::for_each(
        std::begin(q_descriptors), std::end(q_descriptors), [&](auto& d) {
            bool match = false;
            switch (type) {
            case queue::queue_mode::NONE:
                match = (d.mode == type);
                break;
            case queue::queue_mode::TX:
            case queue::queue_mode::RX:
                match = (d.mode == type) || (d.mode == queue::queue_mode::RXTX);
                break;
            case queue::queue_mode::RXTX:
                match = (d.mode == queue::queue_mode::RX)
                        || (d.mode == queue::queue_mode::TX)
                        || (d.mode == queue::queue_mode::RXTX);
                break;
            }
            if (match) workers.push_back(d.worker_id);
        });

    /* Sort and remove any duplicates before returning */
    std::sort(std::begin(workers), std::end(workers));
    workers.erase(std::unique(std::begin(workers), std::end(workers)),
                  std::end(workers));

    return (workers);
}

static int get_port_index(std::string_view id,
                          const driver::generic_driver& driver,
                          const worker::fib* fib)
{
    /* See if this id refers to a port */
    if (auto port_idx = driver.port_index(id); port_idx.has_value()) {
        return (*port_idx);
    }

    /* Maybe it's an interface id; look it up */
    if (auto ifp = fib->find_interface(id); ifp != nullptr) {
        return (to_interface(ifp).port_index());
    }

    /*
     * Not found; return an index we can't possibly have so that we
     * can use the value to filter out all ports in the function above.
     */
    return (-1);
}

static queue::queue_mode to_queue_mode(packet::traffic_direction direction)
{
    switch (direction) {
    case packet::traffic_direction::RX:
        return queue::queue_mode::RX;
    case packet::traffic_direction::TX:
        return queue::queue_mode::TX;
    case packet::traffic_direction::RXTX:
        return queue::queue_mode::RXTX;
    }
}

std::vector<unsigned>
worker_controller::get_worker_ids(std::optional<std::string_view> obj_id,
                                  packet::traffic_direction direction) const
{
    auto mode = to_queue_mode(direction);

    return (obj_id ? get_queue_worker_ids(
                mode, get_port_index(*obj_id, m_driver, m_fib.get()))
                   : get_queue_worker_ids(mode));
}

std::vector<unsigned> worker_controller::get_rx_worker_ids(
    std::optional<std::string_view> obj_id) const
{
    return (obj_id ? get_queue_worker_ids(
                queue::queue_mode::RX,
                get_port_index(*obj_id, m_driver, m_fib.get()))
                   : get_queue_worker_ids(queue::queue_mode::RX));
}

std::vector<unsigned> worker_controller::get_tx_worker_ids(
    std::optional<std::string_view> obj_id) const
{
    return (obj_id ? get_queue_worker_ids(
                queue::queue_mode::TX,
                get_port_index(*obj_id, m_driver, m_fib.get()))
                   : get_queue_worker_ids(queue::queue_mode::TX));
}

template <typename T>
workers::transmit_function to_transmit_function(T tx_function)
{
    return (reinterpret_cast<workers::transmit_function>(tx_function));
}

workers::transmit_function
worker_controller::get_transmit_function(std::string_view port_id) const
{
    auto port_idx = m_driver.port_index(port_id);
    if (!port_idx) {
        /* Also a comment on the caller ;-) */
        return (to_transmit_function(worker::tx_dummy));
    }

    /*
     * We only queue tx packets when we have more than one worker.  Otherwise,
     * we transmit them directly.
     */
    const bool use_direct = rte_lcore_count() <= 2;
    auto info = model::port_info(*port_idx);
    if (std::strcmp(info.driver_name(), "net_ring") == 0) {
        /*
         * For the DPDK driver, we use a portion of the mbuf private area to
         * store the lwip pbuf.  Because the net_ring driver hands transmitted
         * packets directly to another port, we must copy the packet before
         * transmitting to prevent use-after-free bugs.
         */
        return (use_direct ? to_transmit_function(worker::tx_copy_direct)
                           : to_transmit_function(worker::tx_copy_queued));
    }

    return (use_direct ? to_transmit_function(worker::tx_direct)
                       : to_transmit_function(worker::tx_queued));
}

void worker_controller::add_interface(
    std::string_view port_id, const interface::generic_interface& interface)
{
    auto port_idx = m_driver.port_index(port_id);
    if (!port_idx) {
        throw std::runtime_error("Port id " + std::string(port_id)
                                 + " is unknown");
    }

    auto mac = mac_address(interface.mac_address());

    if (m_fib->find_interface(*port_idx, mac)) {
        throw std::runtime_error(
            "Interface with mac = " + libpacket::type::to_string(mac)
            + " on port " + std::string(port_id) + " (idx = "
            + std::to_string(*port_idx) + ") already exists in FIB");
    }

    auto& filter = m_sink_features.get<port::filter>(*port_idx);
    filter.add_mac_address(mac,
                           [&]() { maybe_disable_rxq_tag_detection(filter); });

    auto ifp =
        const_cast<netif*>(std::any_cast<const netif*>(interface.data()));
    auto to_delete = m_fib->insert_interface(*port_idx, mac, ifp);
    m_recycler->writer_add_gc_callback([to_delete]() { delete to_delete; });

    const_cast<net_interface&>(dpdk::to_interface(ifp))
        .set_tx_sink_callback(handle_tx_sink);

    OP_LOG(OP_LOG_DEBUG,
           "Added interface with mac = %s to port %.*s (idx = %u)\n",
           libpacket::type::to_string(mac).c_str(),
           static_cast<int>(port_id.length()),
           port_id.data(),
           *port_idx);
}

void worker_controller::del_interface(
    std::string_view port_id, const interface::generic_interface& interface)
{
    auto port_idx = m_driver.port_index(port_id);
    if (!port_idx) return;

    auto mac = mac_address(interface.mac_address());
    auto to_delete = m_fib->remove_interface(*port_idx, mac);
    m_recycler->writer_add_gc_callback([to_delete]() { delete to_delete; });

    auto& filter = m_sink_features.get<port::filter>(*port_idx);
    filter.del_mac_address(mac,
                           [&]() { maybe_enable_rxq_tag_detection(filter); });

    OP_LOG(OP_LOG_DEBUG,
           "Removed interface with mac = %s to port %.*s (idx = %u)\n",
           libpacket::type::to_string(mac).c_str(),
           static_cast<int>(port_id.length()),
           port_id.data(),
           *port_idx);
}

template <typename Vector, typename Item>
bool contains_match(const Vector& vector, const Item& match)
{
    return (std::any_of(std::begin(vector), std::end(vector), [&](auto& item) {
        return (item == match);
    }));
}

tl::expected<void, int>
worker_controller::add_sink(std::string_view src_id,
                            packet::traffic_direction direction,
                            packet::generic_sink sink)
{
    if (auto port_idx = m_driver.port_index(src_id)) {
        if (direction == packet::traffic_direction::RX
            || direction == packet::traffic_direction::RXTX) {
            if (contains_match(m_fib->get_sinks(*port_idx), sink)) {
                return (tl::make_unexpected(EALREADY));
            }

            OP_LOG(OP_LOG_DEBUG,
                   "Adding rx sink %s to port %.*s (idx = %u)\n",
                   sink.id().c_str(),
                   static_cast<int>(src_id.length()),
                   src_id.data(),
                   *port_idx);

            auto to_delete = m_fib->insert_sink(*port_idx, sink);
            m_recycler->writer_add_gc_callback(
                [to_delete]() { delete to_delete; });

            m_sink_features.update(*m_fib, *port_idx);
        }
        if (direction == packet::traffic_direction::TX
            || direction == packet::traffic_direction::RXTX) {
            // TODO:
            OP_LOG(
                OP_LOG_ERROR,
                "Adding tx sink %s to port %.*s (idx = %u).  Not supported.\n",
                sink.id().c_str(),
                static_cast<int>(src_id.length()),
                src_id.data(),
                *port_idx);
            if (direction == packet::traffic_direction::RXTX) {
                // Remove the Rx sink if error adding Tx sink
                del_sink(src_id, packet::traffic_direction::RX, sink);
            }
            return (tl::make_unexpected(EINVAL));
        }

        return {};
    }

    if (auto ifp = m_fib->find_interface(src_id)) {
        auto& interface = dpdk::to_interface(ifp);
        auto mac = mac_address(ifp->hwaddr);
        if (direction == packet::traffic_direction::RX
            || direction == packet::traffic_direction::RXTX) {
            auto sinks = m_fib->find_interface_sinks(
                interface.port_index(), mac, worker::fib::direction::RX);
            if (!sinks) { return (tl::make_unexpected(EINVAL)); }
            if (contains_match(*sinks, sink)) {
                return (tl::make_unexpected(EALREADY));
            }

            OP_LOG(OP_LOG_DEBUG,
                   "Adding rx sink %s to port %s (idx = %u) interface=%s\n",
                   sink.id().c_str(),
                   interface.port_id().c_str(),
                   interface.port_index(),
                   interface.id().c_str());

            auto to_delete =
                m_fib->insert_interface_sink(interface.port_index(),
                                             mac,
                                             ifp,
                                             worker::fib::direction::RX,
                                             sink);

            m_recycler->writer_add_gc_callback(
                [to_delete]() { delete to_delete; });

            m_sink_features.update(*m_fib, interface.port_index());
        }
        if (direction == packet::traffic_direction::TX
            || direction == packet::traffic_direction::RXTX) {
            auto sinks = m_fib->find_interface_sinks(
                interface.port_index(), mac, worker::fib::direction::TX);
            if (!sinks) {
                if (direction == packet::traffic_direction::RXTX) {
                    // Remove the Rx sink if error adding Tx sink
                    del_sink(src_id, packet::traffic_direction::RX, sink);
                }
                return (tl::make_unexpected(EINVAL));
            }
            if (contains_match(*sinks, sink)) {
                if (direction == packet::traffic_direction::RXTX) {
                    // Remove the Rx sink if error adding Tx sink
                    del_sink(src_id, packet::traffic_direction::RX, sink);
                }
                return (tl::make_unexpected(EALREADY));
            }
            OP_LOG(OP_LOG_DEBUG,
                   "Adding tx sink %s to port %s (idx = %u) interface=%s\n",
                   sink.id().c_str(),
                   interface.port_id().c_str(),
                   interface.port_index(),
                   interface.id().c_str());

            auto to_delete =
                m_fib->insert_interface_sink(interface.port_index(),
                                             mac,
                                             ifp,
                                             worker::fib::direction::TX,
                                             sink);
            m_recycler->writer_add_gc_callback(
                [to_delete]() { delete to_delete; });

            const_cast<net_interface&>(interface).set_tx_sink_data(m_fib.get());
        }
        return {};
    }

    return (tl::make_unexpected(EINVAL));
}

void worker_controller::del_sink(std::string_view src_id,
                                 packet::traffic_direction direction,
                                 packet::generic_sink sink)
{
    if (auto port_idx = m_driver.port_index(src_id)) {
        if (direction == packet::traffic_direction::RX
            || direction == packet::traffic_direction::RXTX) {
            OP_LOG(OP_LOG_DEBUG,
                   "Deleting rx sink %s from port %.*s (idx = %u)\n",
                   sink.id().c_str(),
                   static_cast<int>(src_id.length()),
                   src_id.data(),
                   *port_idx);

            auto to_delete = m_fib->remove_sink(*port_idx, std::move(sink));
            m_recycler->writer_add_gc_callback(
                [to_delete]() { delete to_delete; });

            m_sink_features.update(*m_fib, *port_idx);
        }
        if (direction == packet::traffic_direction::TX
            || direction == packet::traffic_direction::RXTX) {
            // TODO:
        }
        return;
    }

    if (auto ifp = m_fib->find_interface(src_id)) {
        auto& interface = dpdk::to_interface(ifp);
        auto mac = mac_address(ifp->hwaddr);

        if (direction == packet::traffic_direction::RX
            || direction == packet::traffic_direction::RXTX) {
            auto sinks = m_fib->find_interface_sinks(
                interface.port_index(), mac, worker::fib::direction::RX);
            if (sinks && contains_match(*sinks, sink)) {
                OP_LOG(OP_LOG_DEBUG,
                       "Deleting rx sink %s from port %s (idx = %u) interface "
                       "%s\n",
                       sink.id().c_str(),
                       interface.port_id().c_str(),
                       interface.port_index(),
                       interface.id().c_str());

                auto to_delete =
                    m_fib->remove_interface_sink(interface.port_index(),
                                                 mac,
                                                 ifp,
                                                 worker::fib::direction::RX,
                                                 sink);
                m_recycler->writer_add_gc_callback(
                    [to_delete]() { delete to_delete; });

                m_sink_features.update(*m_fib, interface.port_index());
            }
        }
        if (direction == packet::traffic_direction::TX
            || direction == packet::traffic_direction::RXTX) {
            auto sinks = m_fib->find_interface_sinks(
                interface.port_index(), mac, worker::fib::direction::TX);
            if (sinks && contains_match(*sinks, sink)) {
                OP_LOG(OP_LOG_DEBUG,
                       "Deleting tx sink %s from port %s (idx = %u) interface "
                       "%s\n",
                       sink.id().c_str(),
                       interface.port_id().c_str(),
                       interface.port_index(),
                       interface.id().c_str());

                auto to_delete =
                    m_fib->remove_interface_sink(interface.port_index(),
                                                 mac,
                                                 ifp,
                                                 worker::fib::direction::TX,
                                                 sink);

                m_recycler->writer_add_gc_callback(
                    [to_delete]() { delete to_delete; });
            }
        }

        return;
    }
}

/*
 * Find the transmit queue and worker index of the least loaded worker that
 * can transmit on the specified port.
 */
std::pair<uint16_t, unsigned>
get_queue_and_worker_idx(const worker_controller::worker_map& workers,
                         const worker_controller::load_map& loads,
                         uint16_t port_idx)
{
    struct port_comparator
    {
        bool operator()(const worker_controller::worker_map::value_type& left,
                        const worker_controller::worker_map::key_type& right)
        {
            return (left.first.first < right.first);
        }

        bool operator()(const worker_controller::worker_map::key_type& left,
                        const worker_controller::worker_map::value_type& right)
        {
            return (left.first < right.first.first);
        }
    };

    /*
     * Find the range for all of the worker entries for the specified port,
     * e.g. where (port, 0) <= key <= (port, infinity).
     */
    auto range = std::equal_range(std::begin(workers),
                                  std::end(workers),
                                  std::make_pair(port_idx, 0),
                                  port_comparator{});

    /*
     * Now, find the least loaded worker for this port range and return
     * both the worker id and the queue id.
     * Note: our range items contain:
     * first = port/queue index as std::pair<uint16_t, uint16_t>
     * second = worker index
     * We find the minimum over the set and store the results in a tuple
     * containing the minimum load with the associated queue and worker.
     */
    auto load_queue_worker = std::accumulate(
        range.first,
        range.second,
        std::make_tuple(std::numeric_limits<uint64_t>::max(),
                        std::numeric_limits<uint16_t>::max(),
                        std::numeric_limits<unsigned>::max()),
        [&](const auto& tuple,
            const worker_controller::worker_map::value_type& item) {
            auto worker_load = loads.at(item.second);
            if (worker_load < std::get<0>(tuple)) {
                return (std::make_tuple(
                    worker_load, item.first.second, item.second));
            } else {
                return (tuple);
            }
        });

    /* Make sure we found something usable */
    assert(std::get<1>(load_queue_worker)
           != std::numeric_limits<uint16_t>::max());
    assert(std::get<2>(load_queue_worker)
           != std::numeric_limits<unsigned>::max());

    return (std::make_pair(std::get<1>(load_queue_worker),
                           std::get<2>(load_queue_worker)));
}

/* This judges load by interrupts/sec */
uint64_t get_source_load(const packet::generic_source& source)
{
    return ((source.packet_rate() / source.burst_size()).count());
}

std::optional<uint16_t>
find_queue(worker::tib& tib, uint16_t port_idx, std::string_view source_id)
{
    for (auto&& item : tib.get_sources(port_idx)) {
        const auto& key = item->first;
        const auto& source = item->second;
        if (source.id() == source_id) {
            return (std::get<worker::tib::key_queue_idx>(key));
        }
    }
    return (std::nullopt);
}

tl::expected<void, int>
worker_controller::add_source(std::string_view dst_id,
                              packet::generic_source source)
{
    /* Only support port sources for now */
    // TODO: Lookup interface if no port found
    auto port_idx = m_driver.port_index(dst_id);
    if (!port_idx) { return (tl::make_unexpected(EINVAL)); }

    if (find_queue(*m_tib, *port_idx, source.id())) {
        return (tl::make_unexpected(EALREADY));
    }

    auto [queue_idx, worker_idx] =
        get_queue_and_worker_idx(m_tx_workers, m_tx_loads, *port_idx);
    m_tx_loads[worker_idx] += get_source_load(source);

    OP_LOG(OP_LOG_DEBUG,
           "Adding source %s to port %.*s (idx = %u, queue = %u) on worker "
           "%u\n",
           source.id().c_str(),
           static_cast<int>(dst_id.length()),
           dst_id.data(),
           *port_idx,
           queue_idx,
           worker_idx);

    auto to_delete = m_tib->insert_source(
        *port_idx, queue_idx, tx_source(*port_idx, std::move(source)));
    m_recycler->writer_add_gc_callback([to_delete]() { delete to_delete; });

    m_source_features.update(*m_tib, *port_idx);

    return {};
}

void worker_controller::del_source(std::string_view dst_id,
                                   const packet::generic_source& source)
{
    /* Only support port sources for now */
    // TODO: Lookup interface if no port found
    auto port_idx = m_driver.port_index(dst_id);
    if (!port_idx) return;

    auto queue_idx = find_queue(*m_tib, *port_idx, source.id());
    if (!queue_idx) return;

    auto worker_idx = m_tx_workers[std::make_pair(*port_idx, *queue_idx)];
    auto& worker_load = m_tx_loads[worker_idx];
    auto source_load = get_source_load(source);
    if (worker_load > source_load) worker_load -= source_load;

    OP_LOG(OP_LOG_DEBUG,
           "Deleting source %s from port %.*s (idx = %u, queue = %u) on "
           "worker "
           "%u\n",
           source.id().c_str(),
           static_cast<int>(dst_id.length()),
           dst_id.data(),
           *port_idx,
           *queue_idx,
           worker_idx);

    auto to_delete = m_tib->remove_source(*port_idx, *queue_idx, source.id());
    m_recycler->writer_add_gc_callback([to_delete]() { delete to_delete; });

    m_source_features.update(*m_tib, *port_idx);
}

tl::expected<void, int> worker_controller::swap_source(
    std::string_view dst_id,
    const packet::generic_source& outgoing,
    packet::generic_source incoming,
    std::optional<workers::source_swap_function>&& swap_action)
{
    /* Only support port sources for now */
    const auto port_idx = m_driver.port_index(dst_id);
    if (!port_idx) { return (tl::make_unexpected(EINVAL)); }

    if (find_queue(*m_tib, *port_idx, incoming.id())) {
        return (tl::make_unexpected(EALREADY));
    }

    const auto out_queue_idx = find_queue(*m_tib, *port_idx, outgoing.id());
    if (!out_queue_idx) { return (tl::make_unexpected(ENODEV)); }

    /*
     * Figure out which queue/worker to send the incoming source to
     * *after* we remove the outgoing source's load.
     */
    const auto out_worker_idx =
        m_tx_workers[std::make_pair(*port_idx, *out_queue_idx)];
    auto& worker_load = m_tx_loads[out_worker_idx];
    const auto load = get_source_load(outgoing);
    if (worker_load > load) worker_load -= load;

    const auto [in_queue_idx, in_worker_idx] =
        get_queue_and_worker_idx(m_tx_workers, m_tx_loads, *port_idx);
    m_tx_loads[in_worker_idx] += get_source_load(incoming);

    OP_LOG(OP_LOG_DEBUG,
           "Swapping source %s with %s on port %.*s "
           "(idx = %u; queue = %u, worker %u --> queue = %u, worker = %u)\n",
           outgoing.id().c_str(),
           incoming.id().c_str(),
           static_cast<int>(dst_id.length()),
           dst_id.data(),
           *port_idx,
           *out_queue_idx,
           out_worker_idx,
           in_queue_idx,
           in_worker_idx);

    /* Perform the swap. */
    auto to_delete1 =
        m_tib->remove_source(*port_idx, *out_queue_idx, outgoing.id());
    if (swap_action) { (*swap_action)(outgoing, incoming); }
    auto to_delete2 = m_tib->insert_source(
        *port_idx, in_queue_idx, tx_source(*port_idx, std::move(incoming)));
    m_recycler->writer_add_gc_callback([to_delete1, to_delete2]() {
        delete to_delete1;
        delete to_delete2;
    });

    m_source_features.update(*m_tib, *port_idx);

    return {};
}

tl::expected<std::string, int> worker_controller::add_task(
    workers::context ctx,
    std::string_view name,
    event_loop::event_notifier notify,
    event_loop::event_handler&& on_event,
    std::optional<event_loop::delete_handler>&& on_delete,
    std::any&& arg)
{
    /* XXX: We only know about stack tasks right now */
    if (ctx != workers::context::STACK) {
        return (tl::make_unexpected(EINVAL));
    }

    auto id = core::uuid::random();
    auto [it, success] =
        m_tasks.try_emplace(id,
                            name,
                            notify,
                            std::forward<decltype(on_event)>(on_event),
                            std::forward<decltype(on_delete)>(on_delete),
                            std::forward<std::any>(arg));
    if (!success) { return (tl::make_unexpected(EALREADY)); }

    OP_LOG(OP_LOG_DEBUG,
           "Added task %.*s with id = %s\n",
           static_cast<int>(name.length()),
           name.data(),
           core::to_string(id).c_str());

    std::vector<worker::descriptor> tasks{worker::descriptor(
        topology::get_stack_lcore_id(), std::addressof(it->second))};
    m_workers->add_descriptors(tasks);

    return (core::to_string(id));
}

void worker_controller::del_task(std::string_view task_id)
{
    OP_LOG(OP_LOG_DEBUG,
           "Deleting task %.*s\n",
           static_cast<int>(task_id.length()),
           task_id.data());
    auto id = core::uuid(task_id);
    if (auto item = m_tasks.find(id); item != m_tasks.end()) {
        std::vector<worker::descriptor> tasks{worker::descriptor(
            topology::get_stack_lcore_id(), std::addressof(item->second))};
        m_workers->del_descriptors(tasks);
        m_recycler->writer_add_gc_callback([id, this]() { m_tasks.erase(id); });
    }
}

/***
 * Logic for enabling/disabling port features
 ***/

template <typename Function>
bool port_sink_find_if(const worker::fib& fib,
                       size_t port_idx,
                       const Function& filter)
{
    const auto& sinks = fib.get_sinks(port_idx);
    return (std::any_of(std::begin(sinks), std::end(sinks), filter));
}

template <typename Function>
bool interface_sink_find_if(const worker::fib& fib,
                            size_t port_idx,
                            worker::fib::direction dir,
                            const Function& filter)
{
    bool found = false;
    fib.visit_interface_sinks(
        port_idx, dir, [&](netif*, const packet::generic_sink& sink) {
            if (filter(sink)) { found = true; }
            return (!found);
        });
    return (found);
}

template <typename Function>
bool sink_find_if(const worker::fib& fib,
                  size_t port_idx,
                  worker::fib::direction dir,
                  Function&& filter)
{
    return (port_sink_find_if(fib, port_idx, filter)
            || interface_sink_find_if(fib, port_idx, dir, filter));
}

template <>
bool need_sink_feature(const worker::fib& fib,
                       size_t port_idx,
                       const port::filter&)
{
    /*
     * Filter logic is reversed from the others; we need to disable the
     * filter when any sinks are present.
     */
    return (!sink_find_if(
        fib, port_idx, worker::fib::direction::RX, [](const auto&) {
            return (true);
        }));
}

template <>
bool need_sink_feature(const worker::fib& fib,
                       size_t port_idx,
                       const port::packet_type_decoder&)
{
    return (
        sink_find_if(fib,
                     port_idx,
                     worker::fib::direction::RX,
                     [](const packet::generic_sink& sink) {
                         return (sink.uses_feature(
                             packet::sink_feature_flags::packet_type_decode));
                     }));
}

template <>
bool need_sink_feature(const worker::fib& fib,
                       size_t port_idx,
                       const port::rss_hasher&)
{
    return (sink_find_if(fib,
                         port_idx,
                         worker::fib::direction::RX,
                         [](const packet::generic_sink& sink) {
                             return (sink.uses_feature(
                                 packet::sink_feature_flags::rss_hash));
                         }));
}

template <>
bool need_sink_feature(const worker::fib& fib,
                       size_t port_idx,
                       const port::prbs_error_detector&)
{
    return (sink_find_if(
        fib,
        port_idx,
        worker::fib::direction::RX,
        [](const packet::generic_sink& sink) {
            return (sink.uses_feature(
                packet::sink_feature_flags::spirent_prbs_error_detect));
        }));
}

template <>
bool need_sink_feature(const worker::fib& fib,
                       size_t port_idx,
                       const port::signature_decoder&)
{
    return (sink_find_if(
        fib,
        port_idx,
        worker::fib::direction::RX,
        [](const packet::generic_sink& sink) {
            return (sink.uses_feature(
                packet::sink_feature_flags::spirent_signature_decode));
        }));
}

template <>
bool need_sink_feature(const worker::fib& fib,
                       size_t port_idx,
                       const port::timestamper&)
{
    return (sink_find_if(fib,
                         port_idx,
                         worker::fib::direction::RX,
                         [](const packet::generic_sink& sink) {
                             return (sink.uses_feature(
                                 packet::sink_feature_flags::rx_timestamp));
                         }));
}

template <typename Function>
bool source_find_if(const worker::tib& tib, size_t port_idx, Function&& filter)
{
    const auto& range = tib.get_sources(port_idx);
    return (std::any_of(range.first, range.second, [&](const auto& item) {
        return (filter(item->second));
    }));
}

template <>
bool need_source_feature(const worker::tib& tib,
                         size_t port_idx,
                         const port::signature_encoder&)
{
    return (source_find_if(tib, port_idx, [](const dpdk::tx_source& source) {
        return (source.uses_feature(
            packet::source_feature_flags::spirent_signature_encode));
    }));
}

} // namespace openperf::packetio::dpdk
