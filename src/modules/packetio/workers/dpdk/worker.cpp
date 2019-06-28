#include <algorithm>
#include <array>
#include <atomic>  /* XXX: needed to clean up a header order issue in libc/libc++ */
#include <chrono>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <zmq.h>
#include "lwip/netif.h"

#include "core/icp_log.h"
#include "core/icp_thread.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/queue_utils.h"
#include "packetio/memory/dpdk/pbuf_utils.h"
#include "packetio/workers/dpdk/callback.h"
#include "packetio/workers/dpdk/epoll_poller.h"
#include "packetio/workers/dpdk/rx_queue.h"
#include "packetio/workers/dpdk/tx_queue.h"
#include "packetio/workers/dpdk/zmq_socket.h"
#include "packetio/workers/dpdk/worker_api.h"

namespace icp::packetio::dpdk::worker {

const std::string_view endpoint = "inproc://icp_packetio_workers_control";

static constexpr uint16_t pkt_burst_size = 32;
static constexpr int mbuf_prefetch_offset = 4;

using namespace std::chrono_literals;
static constexpr auto poll_timeout = 250ms;

/**
 * We have three states we transition between, based on our messages:
 * init, armed, and running.  We use each struct as a tag for
 * each state.  And we wrap them all in std::variant for ease of use.
 */
struct state_init {};
struct state_armed {};
struct state_running {};

using state = std::variant<state_init, state_armed, state_running>;

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

/**
 * This CRTP based class provides a simple finite state machine framework
 * for controlling worker behavior.
 */
template <typename Derived, typename StateVariant, typename EventVariant>
class finite_state_machine
{
    StateVariant m_state;
public:
    void dispatch(const EventVariant& event)
    {
        Derived& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](auto& state, const auto& event) -> std::optional<StateVariant> {
                return (child.on_event(state, event));
            },
            m_state, event);
        if (next_state) {
            m_state = *next_state;
        }
    }
};

static bool all_pollable(const std::vector<task_ptr>& tasks)
{
    epoll_poller poller;

    auto add_visitor = [&](auto task) -> bool {
                           return (poller.add(task));
                       };

    auto del_visitor = [&](auto task) {
                           poller.del(task);
                       };

    for (auto& task : tasks) {
        if (!std::visit(add_visitor, task)) {
            return (false);
        }
        std::visit(del_visitor, task);
    }
    return (true);
}

static std::pair<size_t, size_t> partition_mbufs(rte_mbuf* incoming[], int length,
                                                 rte_mbuf* unicast[], rte_mbuf* multicast[])
{
    size_t ucast_idx = 0, mcast_idx = 0;
    int i = 0;

    /*
     * Pre-fetching the payload data to the CPU cache is critical for parsing
     * performance.  This series of loops is inteded to keep the cache filled
     * with pending data.
     */

    /* First, prefetch some mbuf payloads. */
    for (i = 0; i < mbuf_prefetch_offset && i < length; i++) {
        rte_prefetch0(rte_pktmbuf_mtod(incoming[i], void*));
    }

    /*
     * Continue pre-fetching the packet data while partitioning the packets
     * based on data that should now be in the local CPU cache.
     */
    for (i = 0; i < (length - mbuf_prefetch_offset); i++) {
        rte_prefetch0(rte_pktmbuf_mtod(incoming[i + mbuf_prefetch_offset], void*));
        auto eth = rte_pktmbuf_mtod(incoming[i], struct ether_hdr*);
        if (is_unicast_ether_addr(&eth->d_addr)) {
            unicast[ucast_idx++] = incoming[i];
        } else {
            multicast[mcast_idx++] = incoming[i];
        }
    }

     /* All prefetches are done; finish parsing the remaining mbufs. */
    for (; i < length; i++) {
        auto eth = rte_pktmbuf_mtod(incoming[i], struct ether_hdr*);
        if (is_unicast_ether_addr(&eth->d_addr)) {
            unicast[ucast_idx++] = incoming[i];
        } else {
            multicast[mcast_idx++] = incoming[i];
        }
    }

    return std::make_pair(ucast_idx, mcast_idx);
}

static uint16_t rx_burst(const fib* fib, const rx_queue* rxq)
{
    static const rte_gro_param gro_params = { .gro_types = RTE_GRO_TCP_IPV4,
                                              .max_flow_num = pkt_burst_size,
                                              .max_item_per_flow = pkt_burst_size };
    /*
     * We pull packets from the port and put them in the receive array.
     * We then sort them into unicast and multicast types.
     */
    std::array<rte_mbuf*, pkt_burst_size> incoming, unicast, nunicast;
    std::array<netif*, pkt_burst_size> interfaces;

    auto n = rte_eth_rx_burst(rxq->port_id(), rxq->queue_id(),
                              incoming.data(), pkt_burst_size);

    if (!n) return (0);

    n = rte_gro_reassemble_burst(incoming.data(), n, &gro_params);

    ICP_LOG(ICP_LOG_TRACE, "Received %d packet%s on %d:%d\n",
            n, n > 1 ? "s" : "", rxq->port_id(), rxq->queue_id());

    auto [nb_ucast, nb_nucast] = partition_mbufs(incoming.data(), n,
                                                 unicast.data(),
                                                 nunicast.data());

    /* Lookup interfaces for unicast packets ... */
    for (size_t i = 0; i < nb_ucast; i++) {
        auto eth = rte_pktmbuf_mtod(unicast[i], struct ether_hdr *);
        interfaces[i] = fib->find(rxq->port_id(), eth->d_addr.addr_bytes);
    }

    /* ... and dispatch */
    for (size_t i = 0; i < nb_ucast; i++) {
        if (!interfaces[i]) {
            rte_pktmbuf_free(unicast[i]);
            continue;
        }

        ICP_LOG(ICP_LOG_TRACE, "Dispatching unicast packet to %c%c%u\n",
                interfaces[i]->name[0], interfaces[i]->name[1],
                interfaces[i]->num);

        auto pbuf = packetio_memory_pbuf_synchronize(unicast[i]);
        if (interfaces[i]->input(pbuf, interfaces[i]) != ERR_OK) {
            pbuf_free(pbuf);
        }
    }

    /* Dispatch all non-unicast packets to all interfaces */
    for (size_t i = 0; i < nb_nucast; i++) {
        /*
         * After we synchronize, use pbuf functions only so that we
         * can keep the mbuf/pbuf synchronized.
         */
        auto pbuf = packetio_memory_pbuf_synchronize(nunicast[i]);
        for (auto ifp : fib->find(rxq->port_id())) {
            ICP_LOG(ICP_LOG_TRACE, "Dispatching non-unicast packet to %c%c%u\n",
                    ifp->name[0], ifp->name[1], ifp->num);

            pbuf_ref(pbuf);
            if (ifp->input(pbuf, ifp) != ERR_OK) {
                pbuf_free(pbuf);
            }
        }
        pbuf_free(pbuf);
    }

    return (n);
}

static uint16_t tx_burst(const tx_queue* txq)
{
    std::array<rte_mbuf*, pkt_burst_size> outgoing;

    auto to_send = rte_ring_dequeue_burst(txq->ring(),
                                          reinterpret_cast<void**>(outgoing.data()),
                                          outgoing.size(), nullptr);

    if (!to_send) return (0);

    auto sent = rte_eth_tx_burst(txq->port_id(), txq->queue_id(),
                                 outgoing.data(), to_send);

    ICP_LOG(ICP_LOG_TRACE, "Transmitted %u of %u packet%s on %u:%u\n",
            sent, to_send, sent > 1 ? "s" : "", txq->port_id(), txq->queue_id());

    size_t retries = 0;
    while (sent < to_send) {
        rte_pause();
        retries++;
        sent += rte_eth_tx_burst(txq->port_id(), txq->queue_id(),
                                 outgoing.data() + sent,
                                 to_send - sent);
    }

    if (retries) {
        ICP_LOG(ICP_LOG_DEBUG, "Transmission required %zu retries on %u:%u\n",
                retries, txq->port_id(), txq->queue_id());
    }

    return (sent);
}

static uint16_t service_task(const fib* fib, const task_ptr& task)
{
    return (std::visit(overloaded_visitor(
                           [](const callback*) -> uint16_t {
                               return (0);  /* noop */
                           },
                           [&](const rx_queue* rxq) -> uint16_t {
                               return (rx_burst(fib, rxq));
                           },
                           [](const tx_queue* txq) -> uint16_t {
                               return (tx_burst(txq));
                           },
                           [](const zmq_socket*) -> uint16_t {
                               return (0);  /* noop */
                           }),
                       task));
}

/**
 * These templated structs allows us to determine if a type has a
 * member function named `enable`.  This and the analogous structs below
 * for `disable` allow us to generically call those functions if they
 * exist without having to explicitly know the type.
 */
template <typename, typename = std::void_t<>>
struct has_enable : std::false_type{};

template <typename T>
struct has_enable<T, std::void_t<decltype(std::declval<T>().enable())>>
    : std::true_type{};

static void enable_task_interrupt(const task_ptr& task)
{
    auto enable_visitor = [](auto t) {
                              if constexpr (has_enable<decltype(t)>::value) {
                                  t->enable();
                              }
                          };
    std::visit(enable_visitor, task);
}

template <typename, typename = std::void_t<>>
struct has_disable : std::false_type{};

template <typename T>
struct has_disable<T, std::void_t<decltype(std::declval<T>().disable())>>
    : std::true_type{};

static void disable_task_interrupt(const task_ptr& task)
{
    auto disable_visitor = [](auto t) {
                               if constexpr (has_disable<decltype(t)>::value) {
                                       t->disable();
                               }
                           };
    std::visit(disable_visitor, task);
}

static void run_pollable(void* control, const fib* fib,
                         const std::vector<task_ptr>& tasks)
{
    epoll_poller poller;

    for (auto& task : tasks) {
        poller.add(task);
    }

    auto ctrl_sock = zmq_socket(control);
    poller.add(&ctrl_sock);

    uint16_t idle_tasks = 0;

    for (;;) {
        do {
            /* Service tasks as fast as possible while we have work. */
            idle_tasks = 0;

            for (auto& task : tasks) {
                auto work = service_task(fib, task);
                idle_tasks += !work;
            }
        } while (idle_tasks < tasks.size());

        /* All tasks are idle; sleep until we have something to do. */
        assert(idle_tasks == tasks.size());

        for (auto& task : tasks) {
            enable_task_interrupt(task);
        }
        poller.wait_for_interrupt();
        for (auto& task : tasks) {
            disable_task_interrupt(task);
        }

        if (ctrl_sock.readable()) break;
    }

    for (auto& task : tasks) {
        poller.del(task);
    }

    poller.del(&ctrl_sock);
}

static void run_spinning(void* control, const fib* fib,
                         const std::vector<task_ptr>& tasks)
{
    while (!icp_socket_has_messages(control)) {
        /*
         * Check the time before servicing any queues.  We want to query our control
         * socket periodically for messages without hammering it.
         */
        using clock = std::chrono::steady_clock;
        auto start = clock::now();

        do {
            uint16_t idle_tasks = 0;

            /* Service queues as fast as possible if any of them have packets. */
            do {
                idle_tasks = 0;
                for (auto& task : tasks) {
                    auto work = service_task(fib, task);
                    idle_tasks += !work;
                }
            } while (idle_tasks < tasks.size());

            /* All queues are idle; take a break from polling */
            rte_delay_us(1);

            /* Keep doing this until we need to query our control socket */
        } while (clock::now() - start > poll_timeout);
    }
}

static void run(void* control, const fib* fib,
                const std::vector<task_ptr>& tasks)
{
    /*
     * XXX: Our control socket is edge triggered, so make sure we drain
     * all of the control messages before jumping into our real run
     * function.  We won't receive any more control interrupts otherwise.
     */
    if (icp_socket_has_messages(control)) return;

    if (all_pollable(tasks)) {
        run_pollable(control, fib, tasks);
    } else {
        run_spinning(control, fib, tasks);
    }
}

class worker : public finite_state_machine<worker, state, command_msg>
{
    void* m_context;
    void* m_control;
    const fib* m_fib;
    std::vector<task_ptr> m_tasks;

    void update_config(const std::vector<descriptor>& descriptors)
    {
        m_tasks.clear();

        for (auto& d: descriptors) {
            if (d.worker_id != rte_lcore_id()) continue;

            std::visit(overloaded_visitor(
                           [&](callback* callback) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding callback %.*s to worker %u\n",
                                       static_cast<int>(callback->name().length()), callback->name().data(),
                                       rte_lcore_id());
                               m_tasks.emplace_back(callback);
                           },
                           [&](rx_queue* rxq) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding RX port queue %u:%u to worker %u\n",
                                       rxq->port_id(), rxq->queue_id(), rte_lcore_id());
                               m_tasks.emplace_back(rxq);
                           },
                           [&](tx_queue* txq) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding TX port queue %u:%u to worker %u\n",
                                       txq->port_id(), txq->queue_id(), rte_lcore_id());
                               m_tasks.emplace_back(txq);
                           },
                           [](zmq_socket*) {
                               /*
                                * Here for completeness, but should never be
                                * configured via an update message.
                                */
                               ICP_LOG(ICP_LOG_ERROR, "Unexpected zmq socket send to worker %u\n",
                                       rte_lcore_id());
                           }),
                       d.task);
        }
    }

public:
    worker(void *context, void* control, const fib* fib)
        : m_context(context)
        , m_control(control)
        , m_fib(fib)
    {}

    /* State transition functions */
    std::optional<state> on_event(state_init&, const configure_msg& msg)
    {
        update_config(msg.descriptors);
        return (msg.descriptors.empty()
                ? std::nullopt
                : std::make_optional(state_armed()));
    }

    std::optional<state> on_event(state_armed&, const start_msg &start)
    {
        icp_task_sync_ping(m_context, start.endpoint.data());
        run(m_control, m_fib, m_tasks);
        return (std::make_optional(state_running()));
    }

    std::optional<state> on_event(state_armed&, const configure_msg& msg)
    {
        update_config(msg.descriptors);
        return (msg.descriptors.empty()
                ? std::nullopt
                : std::make_optional(state_armed()));
    }

    std::optional<state> on_event(state_running&, const stop_msg& stop)
    {
        icp_task_sync_ping(m_context, stop.endpoint.data());
        return (std::make_optional(state_armed()));
    }

    std::optional<state> on_event(state_running&, const configure_msg& msg)
    {
        update_config(msg.descriptors);
        if (msg.descriptors.empty()) {
            return (std::make_optional(state_init()));
        }
        run(m_control, m_fib, m_tasks);
        return (std::make_optional(state_running()));
    }

    /* Generic stop */
    template <typename State>
    std::optional<state> on_event(State&, const stop_msg& stop)
    {
        icp_task_sync_ping(m_context, stop.endpoint.data());
        return std::nullopt;
    }

    /* Generic start */
    template <typename State>
    std::optional<state> on_event(State&, const start_msg& start)
    {
        icp_task_sync_ping(m_context, start.endpoint.data());
        return std::nullopt;
    }
};

int main(void *void_args)
{
    icp_thread_setname(("icp_worker_" + std::to_string(rte_lcore_id())).c_str());

    auto args = reinterpret_cast<main_args*>(void_args);

    void *context = args->context;
    std::unique_ptr<void, icp_socket_deleter> control(
        icp_socket_get_client_subscription(context, endpoint.data(), ""));

    auto me = worker(context, control.get(), args->fib);

    /* XXX: args are unavailable after this point */
    icp_task_sync_ping(context, args->endpoint.data());

    while (auto cmd = recv_message(control.get())) {
        me.dispatch(*cmd);
    }

    ICP_LOG(ICP_LOG_WARNING, "queue worker %d exited\n", rte_lcore_id());

    return (0);
}

}
