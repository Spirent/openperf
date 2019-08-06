#include <chrono>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/physical_port.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/drivers/dpdk/topology_utils.h"
#include "packetio/workers/dpdk/event_loop_adapter.h"
#include "packetio/workers/dpdk/tx_source.h"
#include "packetio/workers/dpdk/worker_tx_functions.h"
#include "packetio/workers/dpdk/worker_queues.h"
#include "packetio/workers/dpdk/worker_controller.h"

namespace icp::packetio::dpdk {

static void launch_workers(void* context, worker::recycler* recycler, const worker::fib* fib)
{
    /* Launch work threads on all of our available worker cores */
    static std::string_view sync_endpoint = "inproc://dpdk_worker_sync";
    auto sync = icp_task_sync_socket(context, sync_endpoint.data());
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
     * Wait until all workers have pinged us back.  If we send out the configuration
     * before all of the workers are ready, they could miss it.
     */
    icp_task_sync_block(&sync, rte_lcore_count() - 1);
}

static std::vector<worker::descriptor> to_worker_descriptors(const std::vector<queue::descriptor>& descriptors,
                                                             const std::vector<worker_controller::txsched_ptr>& tx_schedulers,
                                                             const worker_controller::worker_map& tx_workers,
                                                             const worker::port_queues& queues)
{
    std::vector<worker::descriptor> worker_items;

    for (auto& d: descriptors) {
        assert (d.mode != queue::queue_mode::NONE);
        switch (d.mode) {
        case queue::queue_mode::RX:
            worker_items.emplace_back(d.worker_id, queues[d.port_id].rx(d.queue_id));
            break;
        case queue::queue_mode::TX:
            worker_items.emplace_back(d.worker_id, queues[d.port_id].tx(d.queue_id));
            break;
        case queue::queue_mode::RXTX:
            worker_items.emplace_back(d.worker_id, queues[d.port_id].rx(d.queue_id));
            worker_items.emplace_back(d.worker_id, queues[d.port_id].tx(d.queue_id));
            break;
        default:
            break;
        }
    }

    for (auto& sched_ptr : tx_schedulers) {
        auto idx = tx_workers.at(std::make_pair(sched_ptr->port_id(), sched_ptr->queue_id()));
        worker_items.emplace_back(idx, sched_ptr.get());
    }

    return (worker_items);
}

static std::vector<queue::descriptor> get_queue_descriptors(std::vector<model::port_info>& port_info)
{
    auto q_descriptors = topology::queue_distribute(port_info);

    /*
     * XXX: If we have only one worker thread, then everything should use the
     * direct transmit functions.  Hence, we don't need any tx queues.
     */
    if (rte_lcore_count() <= 2) {
        /* Filter out all tx queues; we don't need them */
        q_descriptors.erase(std::remove_if(begin(q_descriptors), end(q_descriptors),
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

static unsigned num_workers()
{
    return (rte_lcore_count() - 1);
}

static int handle_recycler_timeout(const icp_event_data*, void* arg)
{
    auto recycler = reinterpret_cast<worker::recycler*>(arg);
    recycler->writer_process_gc_callbacks();
    return (0);
}

static void setup_recycler_callback(icp::core::event_loop& loop, worker::recycler* recycler)
{
    using namespace std::chrono_literals;
    std::chrono::duration<uint64_t, std::nano> timeout = 5s;

    struct icp_event_callbacks callbacks = {
        .on_read = handle_recycler_timeout
    };

    loop.add(timeout.count(), &callbacks, recycler);
}

worker_controller::worker_controller(void* context,
                                     icp::core::event_loop& loop,
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
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        m_recycler->writer_add_reader(lcore_id);
    }

    /* And create the periodic callback */
    setup_recycler_callback(loop, m_recycler.get());

    /* We need port information to setup our workers, so get it */
    uint16_t port_id = 0;
    std::vector<model::port_info> port_info;
    RTE_ETH_FOREACH_DEV(port_id) {
        port_info.emplace_back(model::port_info(port_id));
    }

    /* Construct our necessary transmit schedulers and metadata */
    for (auto& d : topology::queue_distribute(port_info)) {
        if (d.mode == queue::queue_mode::RX) continue;
        m_tx_schedulers.push_back(std::make_unique<tx_scheduler>(*m_tib, d.port_id, d.queue_id));
        m_tx_loads.emplace(d.worker_id, 0);
        m_tx_workers.emplace(std::make_pair(d.port_id, d.queue_id), d.worker_id);
    }

    /* Construct our necessary port queues */
    auto q_descriptors = get_queue_descriptors(port_info);
    auto& queues = worker::port_queues::instance();
    queues.setup(q_descriptors);

    /* Distribute queues and schedulers to workers */
    m_workers->add_descriptors(to_worker_descriptors(q_descriptors,
                                                     m_tx_schedulers, m_tx_workers,
                                                     queues));

    /* And start them */
    m_workers->start(m_context, num_workers());
}

worker_controller::~worker_controller()
{
    if (!m_workers) return;

    m_workers->stop(m_context, num_workers());
    m_recycler->writer_process_gc_callbacks();
    rte_eal_mp_wait_lcore();
    worker::port_queues::instance().unset();
}

worker_controller::worker_controller(worker_controller&& other)
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
{}

worker_controller& worker_controller::operator=(worker_controller&& other)
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
    }
    return (*this);
}

template <typename T>
workers::transmit_function to_transmit_function(T tx_function)
{
    return (reinterpret_cast<workers::transmit_function>(tx_function));
}

workers::transmit_function worker_controller::get_transmit_function(std::string_view port_id) const
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
        return (use_direct
                ? to_transmit_function(worker::tx_copy_direct)
                : to_transmit_function(worker::tx_copy_queued));
    }

    return (use_direct
            ? to_transmit_function(worker::tx_direct)
            : to_transmit_function(worker::tx_queued));
}

void worker_controller::add_interface(std::string_view port_id, std::any interface)
{
    auto port_idx = m_driver.port_index(port_id);
    if (!port_idx) {
        throw std::runtime_error("Port id " + std::string(port_id) + " is unknown");
    }

    /* We really only expect one type here */
    auto ifp = std::any_cast<netif*>(interface);
    auto mac = net::mac_address(ifp->hwaddr);

    if (m_fib->find_interface(*port_idx, mac)) {
        throw std::runtime_error("Interface with mac = "
                                 + net::to_string(mac) + " on port "
                                 + std::string(port_id) + " (idx = "
                                 + std::to_string(*port_idx) + ") already exists in FIB");
    }

    auto port = model::physical_port(*port_idx, port_id);
    port.add_mac_address(mac);
    auto to_delete = m_fib->insert_interface(*port_idx, mac, ifp);
    m_recycler->writer_add_gc_callback([to_delete](){ delete to_delete; });

    ICP_LOG(ICP_LOG_DEBUG, "Added interface with mac = %s to port %.*s (idx = %u)\n",
            net::to_string(mac).c_str(),
            static_cast<int>(port_id.length()), port_id.data(),
            *port_idx);
}

void worker_controller::del_interface(std::string_view port_id, std::any interface)
{
    auto port_idx = m_driver.port_index(port_id);
    if (!port_idx) return;

    auto ifp = std::any_cast<netif*>(interface);
    auto mac = net::mac_address(ifp->hwaddr);
    auto to_delete = m_fib->remove_interface(*port_idx, mac);
    m_recycler->writer_add_gc_callback([to_delete](){ delete to_delete; });
    model::physical_port(*port_idx, port_id).del_mac_address(mac);

    ICP_LOG(ICP_LOG_DEBUG, "Removed interface with mac = %s to port %.*s (idx = %u)\n",
            net::to_string(mac).c_str(),
            static_cast<int>(port_id.length()), port_id.data(),
            *port_idx);
}

template <typename Vector, typename Item>
bool contains_match(const Vector& vector, const Item& match)
{
    for (auto& item : vector) {
        if (item == match) return (true);
    }
    return (false);
}

tl::expected<void, int> worker_controller::add_sink(std::string_view src_id,
                                                    packets::generic_sink sink)
{
    /* Only support port sinks for now */
    auto port_idx = m_driver.port_index(src_id);
    if (!port_idx) {
        return (tl::make_unexpected(EINVAL));
    }

    if (contains_match(m_fib->get_sinks(*port_idx), sink)) {
        return (tl::make_unexpected(EALREADY));
    }

    ICP_LOG(ICP_LOG_DEBUG, "Adding sink %s to port %.*s (idx = %u)\n",
            sink.id().c_str(),
            static_cast<int>(src_id.length()), src_id.data(),
            *port_idx);

    auto to_delete = m_fib->insert_sink(*port_idx, std::move(sink));
    m_recycler->writer_add_gc_callback([to_delete](){ delete to_delete; });

    return {};
}

void worker_controller::del_sink(std::string_view src_id, packets::generic_sink sink)
{
    /* Only support port sinks for now */
    auto port_idx = m_driver.port_index(src_id);
    if (!port_idx) return;

    ICP_LOG(ICP_LOG_DEBUG, "Deleting sink %s from port %.*s (idx = %u)\n",
            sink.id().c_str(),
            static_cast<int>(src_id.length()), src_id.data(),
            *port_idx);

    auto to_delete = m_fib->remove_sink(*port_idx, std::move(sink));
    m_recycler->writer_add_gc_callback([to_delete](){ delete to_delete; });
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
    struct port_comparator {
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
    auto load_queue_worker = std::accumulate(range.first, range.second,
                                             std::make_tuple(std::numeric_limits<uint64_t>::max(),
                                                             std::numeric_limits<uint16_t>::max(),
                                                             std::numeric_limits<unsigned>::max()),
                                             [&](const auto& tuple, const worker_controller::worker_map::value_type& item) {
                                                 auto worker_load = loads.at(item.second);
                                                 if (worker_load < std::get<0>(tuple)) {
                                                     return (std::make_tuple(worker_load,
                                                                             item.first.second,
                                                                             item.second));
                                                 } else {
                                                     return (tuple);
                                                 }
                                             });

    /* Make sure we found something usable */
    assert(std::get<1>(load_queue_worker) != std::numeric_limits<uint16_t>::max());
    assert(std::get<2>(load_queue_worker) != std::numeric_limits<unsigned>::max());

    return (std::make_pair(std::get<1>(load_queue_worker),
                           std::get<2>(load_queue_worker)));
}

/* This judges load by interrupts/sec */
uint64_t get_source_load(const packets::generic_source& source)
{
    return ((source.packet_rate() / source.burst_size()).count());
}

std::optional<uint16_t> find_queue(worker::tib& tib, uint16_t port_idx,
                                   std::string_view source_id)
{
    for (const auto& [key, source] : tib.get_sources(port_idx)) {
        if (source.id() == source_id) {
            return (std::get<worker::tib::key_queue_idx>(key));
        }
    }
    return (std::nullopt);
}

tl::expected<void, int> worker_controller::add_source(std::string_view dst_id,
                                                      packets::generic_source source)
{
    /* Only support port sources for now */
    auto port_idx = m_driver.port_index(dst_id);
    if (!port_idx) {
        return (tl::make_unexpected(EINVAL));
    }

    if (find_queue(*m_tib, *port_idx, source.id())) {
        return (tl::make_unexpected(EALREADY));
    }

    auto [queue_idx, worker_idx] = get_queue_and_worker_idx(m_tx_workers, m_tx_loads, *port_idx);
    m_tx_loads[worker_idx] += get_source_load(source);

    ICP_LOG(ICP_LOG_DEBUG, "Adding source %s to port %.*s (idx = %u, queue = %u) on worker %u\n",
            source.id().c_str(),
            static_cast<int>(dst_id.length()), dst_id.data(),
            *port_idx, queue_idx, worker_idx);

    auto to_delete = m_tib->insert_source(*port_idx, queue_idx,
                                          tx_source(*port_idx, std::move(source)));
    m_recycler->writer_add_gc_callback([to_delete](){ delete to_delete; });

    return {};
}

void worker_controller::del_source(std::string_view dst_id, packets::generic_source source)
{
    /* Only support port sources for now */
    auto port_idx = m_driver.port_index(dst_id);
    if (!port_idx) return;

    auto queue_idx = find_queue(*m_tib, *port_idx, source.id());
    if (!queue_idx) return;

    auto worker_idx = m_tx_workers[std::make_pair(*port_idx, *queue_idx)];
    auto& worker_load = m_tx_loads[worker_idx];
    auto source_load = get_source_load(source);
    if (worker_load > source_load) worker_load -= source_load;

    ICP_LOG(ICP_LOG_DEBUG, "Deleting source %s from port %.*s (idx = %u, queue = %u) on worker %u\n",
            source.id().c_str(),
            static_cast<int>(dst_id.length()), dst_id.data(),
            *port_idx, *queue_idx, worker_idx);

    auto to_delete = m_tib->remove_source(*port_idx, *queue_idx, source.id());
    m_recycler->writer_add_gc_callback([to_delete](){ delete to_delete; });
}

tl::expected<std::string, int> worker_controller::add_task(workers::context ctx,
                                                           std::string_view name,
                                                           event_loop::event_notifier notify,
                                                           event_loop::event_handler on_event,
                                                           std::optional<event_loop::delete_handler> on_delete,
                                                           std::any arg)
{
    /* XXX: We only know about stack tasks right now */
    if (ctx != workers::context::STACK) {
        return (tl::make_unexpected(EINVAL));
    }

    auto id = core::uuid::random();
    auto [it, success] = m_tasks.try_emplace(id, name, notify, on_event, on_delete, arg);
    if (!success) {
        return (tl::make_unexpected(EALREADY));
    }

    ICP_LOG(ICP_LOG_DEBUG, "Added task %.*s with id = %s\n",
            static_cast<int>(name.length()), name.data(), core::to_string(id).c_str());

    std::vector<worker::descriptor> tasks { worker::descriptor(topology::get_stack_lcore_id(),
                                                               std::addressof(it->second)) };
    m_workers->add_descriptors(tasks);

    return (core::to_string(id));
}

void worker_controller::del_task(std::string_view task_id)
{
    ICP_LOG(ICP_LOG_DEBUG, "Deleting task %.*s\n",
            static_cast<int>(task_id.length()), task_id.data());
    auto id = core::uuid(task_id);
    if (auto item = m_tasks.find(id); item != m_tasks.end()) {
        std::vector<worker::descriptor> tasks { worker::descriptor(topology::get_stack_lcore_id(),
                                                                   std::addressof(item->second)) };
        m_workers->del_descriptors(tasks);
        m_recycler->writer_add_gc_callback([id,this](){ m_tasks.erase(id); });
    }
}

}
