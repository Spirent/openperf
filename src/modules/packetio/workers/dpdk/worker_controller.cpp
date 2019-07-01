#include "core/icp_uuid.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/physical_port.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/drivers/dpdk/topology_utils.h"
#include "packetio/workers/dpdk/event_loop_adapter.h"
#include "packetio/workers/dpdk/worker_tx_functions.h"
#include "packetio/workers/dpdk/worker_queues.h"
#include "packetio/workers/dpdk/worker_controller.h"

namespace icp::packetio::dpdk {

static void launch_workers(void* context, worker::fib* fib)
{
    /* Launch work threads on all of our available worker cores */
    static std::string_view sync_endpoint = "inproc://dpdk_worker_sync";
    auto sync = icp_task_sync_socket(context, sync_endpoint.data());
    struct worker::main_args args = {
        .context = context,
        .endpoint = sync_endpoint.data(),
        .fib = fib
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

    return (worker_items);
}

static std::vector<queue::descriptor> get_queue_descriptors(std::vector<model::port_info>& port_info)
{
    auto q_descriptors = topology::queue_distribute(port_info);

    /*
     * XXX: If we have only one worker thread, then everything should use the
     * direct transmit functions.  Hence,  we don't need any tx queues.
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

worker_controller::worker_controller(void* context,
                                     driver::generic_driver& driver)
    : m_context(context)
    , m_workers(std::make_unique<worker::client>(context))
    , m_driver(driver)
    , m_fib(std::make_unique<worker::fib>())
{
    launch_workers(context, m_fib.get());

    /* Construct our necessary port queues */
    uint16_t port_id = 0;
    std::vector<model::port_info> port_info;
    RTE_ETH_FOREACH_DEV(port_id) {
        port_info.emplace_back(model::port_info(port_id));
    }

    auto q_descriptors = get_queue_descriptors(port_info);

    auto& queues = worker::port_queues::instance();
    queues.setup(q_descriptors);

    /* Distribute them to our waiting workers */
    m_workers->add_descriptors(to_worker_descriptors(q_descriptors, queues));

    /* And start them */
    m_workers->start(m_context, num_workers());
}

worker_controller::~worker_controller()
{
    if (!m_workers) return;

    m_workers->stop(m_context, num_workers());
    rte_eal_mp_wait_lcore();
    worker::port_queues::instance().unset();
}

worker_controller::worker_controller(worker_controller&& other)
    : m_context(other.m_context)
    , m_workers(std::move(other.m_workers))
    , m_driver(other.m_driver)
    , m_fib(std::move(other.m_fib))
    , m_tasks(std::move(other.m_tasks))
    , m_loops(std::move(other.m_loops))
{}

worker_controller& worker_controller::operator=(worker_controller&& other)
{
    if (this != &other) {
        m_context = other.m_context;
        m_workers = std::move(other.m_workers);
        m_driver = other.m_driver;
        m_fib = std::move(other.m_fib);
        m_tasks = std::move(other.m_tasks);
        m_loops = std::move(other.m_loops);
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

    if (m_fib->find(*port_idx, mac)) {
        throw std::runtime_error("Interface with mac = "
                                 + net::to_string(mac) + " on port "
                                 + std::string(port_id) + " (idx = "
                                 + std::to_string(*port_idx) + ") already exists in FIB");
    }

    auto port = model::physical_port(*port_idx, port_id);
    port.add_mac_address(mac);
    if (!m_fib->insert(*port_idx, mac, ifp)) {
        port.del_mac_address(mac);
        throw std::runtime_error("Could not insert mac = "
                                 + net::to_string(mac) + " for port "
                                 + std::string(port_id) + "(idx = "
                                 + std::to_string(*port_idx) + ") into FIB");
    }

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

    if (!m_fib->remove(*port_idx, mac, ifp)) {
        throw std::runtime_error("Could not remove mac = "
                                 + net::to_string(mac) + " for port "
                                 + std::string(port_id) + " (idx = "
                                 + std::to_string(*port_idx) + ") from FIB");
    }

    model::physical_port(*port_idx, port_id).del_mac_address(mac);

    ICP_LOG(ICP_LOG_DEBUG, "Removed interface with mac = %s to port %.*s (idx = %u)\n",
            net::to_string(mac).c_str(),
            static_cast<int>(port_id.length()), port_id.data(),
            *port_idx);
}

tl::expected<std::string, int> worker_controller::add_task(workers::context ctx,
                                                           std::string_view name,
                                                           event_loop::event_notifier notify,
                                                           event_loop::callback_function callback,
                                                           std::any arg)
{
    /* XXX: We only know about stack tasks right now */
    if (ctx != workers::context::STACK) {
        return (tl::make_unexpected(EINVAL));
    }

    if (m_loops.find(ctx) == m_loops.end()) {
        m_loops.emplace(ctx, worker::event_loop_adapter(m_context, ctx));
    }
    auto& loop = m_loops.at(ctx);

    auto id = core::uuid::random();
    auto [it, success] = m_tasks.try_emplace(id, loop, name, notify, callback, arg);
    if (!success) {
        return (tl::make_unexpected(-1));
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
    if (auto item = m_tasks.find(core::uuid(task_id)); item != m_tasks.end()) {
        std::vector<worker::descriptor> tasks { worker::descriptor(topology::get_stack_lcore_id(),
                                                                   std::addressof(item->second)) };
        m_workers->del_descriptors(tasks);
        m_tasks.erase(item);
    }
}

}
