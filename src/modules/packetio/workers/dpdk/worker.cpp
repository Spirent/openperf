#include <algorithm>
#include <array>
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
#include "packetio/workers/dpdk/event_loop_adapter.h"
#include "packetio/workers/dpdk/rx_queue.h"
#include "packetio/workers/dpdk/tx_queue.h"
#include "packetio/workers/dpdk/tx_scheduler.h"
#include "packetio/workers/dpdk/zmq_socket.h"
#include "packetio/workers/dpdk/worker_api.h"

namespace icp::packetio::dpdk::worker {

const std::string_view endpoint = "inproc://icp_packetio_workers_control";

static constexpr int mbuf_prefetch_offset = 4;

/**
 * We only have two states we transition between, based on our messages:
 * stopped and started.  We use each struct as a tag for each state.  And
 * we wrap them all in std::variant for ease of use.
 */
struct state_stopped {};
struct state_started {};

using state = std::variant<state_stopped, state_started>;

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

static std::pair<size_t, size_t> partition_mbufs(rte_mbuf* const incoming[], int length,
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
        auto eth = rte_pktmbuf_mtod(incoming[i], struct rte_ether_hdr*);
        if (rte_is_unicast_ether_addr(&eth->d_addr)) {
            unicast[ucast_idx++] = incoming[i];
        } else {
            multicast[mcast_idx++] = incoming[i];
        }
    }

     /* All prefetches are done; finish parsing the remaining mbufs. */
    for (; i < length; i++) {
        auto eth = rte_pktmbuf_mtod(incoming[i], struct rte_ether_hdr*);
        if (rte_is_unicast_ether_addr(&eth->d_addr)) {
            unicast[ucast_idx++] = incoming[i];
        } else {
            multicast[mcast_idx++] = incoming[i];
        }
    }

    return std::make_pair(ucast_idx, mcast_idx);
}

static void rx_interface_dispatch(const fib* fib, const rx_queue* rxq,
                                  rte_mbuf* const incoming[], uint16_t n)
{
    std::array<rte_mbuf*, pkt_burst_size> unicast, nunicast;
    std::array<netif*, pkt_burst_size> interfaces;

    auto [nb_ucast, nb_nucast] = partition_mbufs(incoming, n,
                                                 unicast.data(),
                                                 nunicast.data());

    /* Lookup interfaces for unicast packets ... */
    for (size_t i = 0; i < nb_ucast; i++) {
        auto eth = rte_pktmbuf_mtod(unicast[i], struct rte_ether_hdr *);
        interfaces[i] = fib->find_interface(rxq->port_id(), eth->d_addr.addr_bytes);
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
        for (auto [idx, ifp] : fib->get_interfaces(rxq->port_id())) {
            ICP_LOG(ICP_LOG_TRACE, "Dispatching non-unicast packet to %c%c%u\n",
                    ifp->name[0], ifp->name[1], ifp->num);

            pbuf_ref(pbuf);
            if (ifp->input(pbuf, ifp) != ERR_OK) {
                pbuf_free(pbuf);
            }
        }
        pbuf_free(pbuf);
    }
}

static void rx_sink_dispatch(const fib* fib, const rx_queue* rxq,
                             rte_mbuf* const incoming[], uint16_t n)
{
    for(auto& sink : fib->get_sinks(rxq->port_id())) {
        ICP_LOG(ICP_LOG_TRACE, "Dispatching packets to sink %s\n", sink.id().c_str());
        sink.push(reinterpret_cast<packets::packet_buffer* const*>(incoming), n);
    }
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
    std::array<rte_mbuf*, pkt_burst_size> incoming;

    auto n = rte_eth_rx_burst(rxq->port_id(), rxq->queue_id(),
                              incoming.data(), pkt_burst_size);

    if (!n) return (0);

    n = rte_gro_reassemble_burst(incoming.data(), n, &gro_params);

    ICP_LOG(ICP_LOG_TRACE, "Received %d packet%s on %d:%d\n",
            n, n > 1 ? "s" : "", rxq->port_id(), rxq->queue_id());

    /*
     * Bump the reference count on all packets.  We don't want the stack
     * to drop them before we can dispatch them to our sinks.
     */
    for (uint16_t i = 0; i < n; i++) {
        rte_pktmbuf_refcnt_update(incoming[i], 1);
    }

    /* Dispatch packets to stack interfaces first */
    rx_interface_dispatch(fib, rxq, incoming.data(), n);

    /* Then, dispatch packets to any port sinks */
    rx_sink_dispatch(fib, rxq, incoming.data(), n);

    /* Finally, free all of our packets.  This is due to the refcnt bump above */
    for (uint16_t i = 0; i < n; i++) {
        rte_pktmbuf_free(incoming[i]);
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

static uint16_t service_event(event_loop::generic_event_loop& loop,
                              const fib* fib, const task_ptr& task)
{
    return (std::visit(overloaded_visitor(
                           [&](const callback* cb) -> uint16_t {
                               const_cast<callback*>(cb)->run_callback(loop);
                               return (0);
                           },
                           [&](const rx_queue* rxq) -> uint16_t {
                               return (rx_burst(fib, rxq));
                           },
                           [](const tx_queue* txq) -> uint16_t {
                               return (tx_burst(txq));
                           },
                           [](const tx_scheduler* scheduler) -> uint16_t {
                               const_cast<tx_scheduler*>(scheduler)->run();
                               return (0);
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

static void enable_event_interrupt(const task_ptr& task)
{
    auto enable_visitor = [](auto t) {
                              if constexpr (has_enable<decltype(*t)>::value) {
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

static void disable_event_interrupt(const task_ptr& task)
{
    auto disable_visitor = [](auto t) {
                               if constexpr (has_disable<decltype(*t)>::value) {
                                       t->disable();
                               }
                           };
    std::visit(disable_visitor, task);
}

struct run_args {
    void* control;
    recycler* recycler;
    event_loop::generic_event_loop& loop;
    const fib* fib;
    const std::vector<task_ptr>& rx_queues;
    const std::vector<task_ptr>& pollables;
};

static void run_pollable(run_args&& args)
{
    bool messages = false;
    auto ctrl_sock = zmq_socket(args.control, &messages);
    auto& loop_adapter = args.loop.get_reference<event_loop_adapter>();
    auto poller = epoll_poller();

    poller.add(&ctrl_sock);

    for (auto& q : args.rx_queues) {
        poller.add(q);
    }

    for (auto& p : args.pollables) {
        poller.add(p);
    }

    /*
    * Interrupt support on some DPDK NICs appears to be flaky in that
    * enabling interrupts with packets in the queue causes no future
    * interrupt to be generated.  So, make sure the queue is empty
    * after we enable the interrupt.  We can't guarantee we enabled
    * interrupts on an empty queue otherwise.
    * XXX: Enabling interrupts on some platforms is SLOW!
    */
    for (auto& q : args.rx_queues) {
        do {
            enable_event_interrupt(q);
        } while (service_event(args.loop, args.fib, q));
    }

    while (!messages) {
        auto& events = poller.poll();
        {
            auto guard = packetio::recycle::guard(*args.recycler, rte_lcore_id());
            for (auto& event : events) {
                std::visit(overloaded_visitor(
                               [&](callback *cb) {
                                   cb->run_callback(args.loop);
                               },
                               [&](rx_queue* rxq) {
                                   while (rx_burst(args.fib, rxq) == pkt_burst_size)
                                       ;
                                   do {
                                       rxq->enable();
                                   } while (rx_burst(args.fib, rxq));
                               },
                               [](tx_queue* txq) {
                                   while (tx_burst(txq) == pkt_burst_size)
                                       ;
                                   txq->enable();
                               },
                               [](tx_scheduler* scheduler) {
                                   scheduler->run();
                               },
                               [](zmq_socket*) {
                                   /* noop */
                               }),
                           event);
            }
        }
        /* Perform all loop updates before exiting or restarting the loop. */
        loop_adapter.update_poller(poller);
    }

    for (auto& q : args.rx_queues) {
        disable_event_interrupt(q);
        poller.del(q);
    }

    for (auto& p : args.pollables) {
        poller.del(p);
    }

    poller.del(&ctrl_sock);
}

static void run_spinning(run_args&& args)
{
    bool messages = false;
    auto ctrl_sock = zmq_socket(args.control, &messages);
    auto& loop_adapter = args.loop.get_reference<event_loop_adapter>();
    auto poller = epoll_poller();

    poller.add(&ctrl_sock);

    for (auto& p : args.pollables) {
        poller.add(p);
    }

    while (!messages) {
        args.recycler->reader_checkpoint(rte_lcore_id());

        /* Service queues as fast as possible if any of them have packets. */
        uint16_t pkts;
        do {
            pkts = 0;
            for (auto& q : args.rx_queues) {
                pkts += service_event(args.loop, args.fib, q);
            }
        } while (pkts);

        /* All queues are idle; check callbacks */
        for (auto& event : poller.poll(0)) {
             service_event(args.loop, args.fib, event);
        }

        /* Perform all loop updates before exiting or restarting the loop. */
        loop_adapter.update_poller(poller);
    }

    for (auto& p : args.pollables) {
        poller.del(p);
    }

    poller.del(&ctrl_sock);
}

static void run(run_args&& args)
{
    /*
     * XXX: Our control socket is edge triggered, so make sure we drain
     * all of the control messages before jumping into our real run
     * function.  We won't receive any more control interrupts otherwise.
     */
    if (icp_socket_has_messages(args.control)) return;

    if (args.rx_queues.empty() || all_pollable(args.rx_queues)) {
        run_pollable(std::move(args));
    } else {
        run_spinning(std::move(args));
    }
}

class worker : public finite_state_machine<worker, state, command_msg>
{
    void* m_context;
    void* m_control;
    recycler* m_recycler;
    const fib* m_fib;
    event_loop::generic_event_loop m_loop;

    /**
     * Workers deal with a number of different event sources.  All of those
     * sources are interrupt driven with the possible exception of RX queues.
     * Hence, we need to separate rx queues from our other event types in
     * order to handle them properly.
     */
    std::vector<task_ptr> m_rx_queues;
    std::vector<task_ptr> m_pollables;

    void add_config(const std::vector<descriptor>& descriptors)
    {
        for (auto& d: descriptors) {
            if (d.worker_id != rte_lcore_id()) continue;

            std::visit(overloaded_visitor(
                           [&](callback* callback) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding task %.*s to worker %u\n",
                                       static_cast<int>(callback->name().length()), callback->name().data(),
                                       rte_lcore_id());
                               m_pollables.emplace_back(callback);
                           },
                           [&](rx_queue* rxq) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding RX port queue %u:%u to worker %u\n",
                                       rxq->port_id(), rxq->queue_id(), rte_lcore_id());
                               m_rx_queues.emplace_back(rxq);
                           },
                           [&](tx_queue* txq) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding TX port queue %u:%u to worker %u\n",
                                       txq->port_id(), txq->queue_id(), rte_lcore_id());
                               m_pollables.emplace_back(txq);
                           },
                           [&](tx_scheduler* scheduler) {
                               ICP_LOG(ICP_LOG_DEBUG, "Adding TX port scheduler %u:%u to worker %u\n",
                                       scheduler->port_id(), scheduler->queue_id(), rte_lcore_id());
                               m_pollables.emplace_back(scheduler);
                           }),
                       d.ptr);
        }
    }

    void del_config(const std::vector<descriptor>& descriptors)
    {
        for (auto& d: descriptors) {
            if (d.worker_id != rte_lcore_id()) continue;

            /*
             * XXX: Careful!  These pointers have likely been deleted by their owners at this point,
             * so don't dereference them!
             */
            std::visit(overloaded_visitor(
                           [&](callback* cb) {
                               ICP_LOG(ICP_LOG_DEBUG, "Removing task from worker %u\n",
                                       rte_lcore_id());
                               m_pollables.erase(std::remove(std::begin(m_pollables),
                                                             std::end(m_pollables),
                                                             task_ptr{cb}),
                                                 std::end(m_pollables));
                           },
                           [&](rx_queue* rxq) {
                               ICP_LOG(ICP_LOG_DEBUG, "Removing RX port queue from worker %u\n",
                                       rte_lcore_id());
                               m_rx_queues.erase(std::remove(std::begin(m_rx_queues),
                                                          std::end(m_rx_queues),
                                                             task_ptr{rxq}),
                                              std::end(m_rx_queues));
                           },
                           [&](tx_queue* txq) {
                               ICP_LOG(ICP_LOG_DEBUG, "Removing TX port queue from worker %u\n",
                                       rte_lcore_id());
                               m_pollables.erase(std::remove(std::begin(m_pollables),
                                                             std::end(m_pollables),
                                                             task_ptr{txq}),
                                              std::end(m_pollables));
                           },
                           [&](tx_scheduler* scheduler) {
                               ICP_LOG(ICP_LOG_DEBUG, "Removing TX port scheduler from worker %u\n",
                                       rte_lcore_id());
                               m_pollables.erase(std::remove(std::begin(m_pollables),
                                                             std::end(m_pollables),
                                                             task_ptr{scheduler}),
                                              std::end(m_pollables));
                           }),
                       d.ptr);
        }
    }

    run_args make_run_args()
    {
        return (run_args{
                .control = m_control,
                .recycler = m_recycler,
                .loop = m_loop,
                .fib = m_fib,
                .rx_queues = m_rx_queues,
                .pollables = m_pollables
            });
    }

public:
    worker(void *context, void* control, recycler* recycler, const fib* fib)
        : m_context(context)
        , m_control(control)
        , m_recycler(recycler)
        , m_fib(fib)
        , m_loop(event_loop_adapter{})
    {}

    /* State transition functions */
    std::optional<state> on_event(state_stopped&, const add_descriptors_msg& add)
    {
        add_config(add.descriptors);
        return (std::nullopt);
    }

    std::optional<state> on_event(state_stopped&, const del_descriptors_msg& del)
    {
        del_config(del.descriptors);
        return (std::nullopt);
    }

    std::optional<state> on_event(state_started&, const add_descriptors_msg& add)
    {
        add_config(add.descriptors);

        run(make_run_args());
        return (std::nullopt);
    }

    std::optional<state> on_event(state_started&, const del_descriptors_msg& del)
    {
        del_config(del.descriptors);
        run(make_run_args());
        return (std::nullopt);
    }

    /* Generic state transition functions */
    template <typename State>
    std::optional<state> on_event(State&, const start_msg& start)
    {
        icp_task_sync_ping(m_context, start.endpoint.data());
        run(make_run_args());
        return (std::make_optional(state_started{}));
    }

    template <typename State>
    std::optional<state> on_event(State&, const stop_msg& stop)
    {
        icp_task_sync_ping(m_context, stop.endpoint.data());
        return (std::make_optional(state_stopped{}));
    }
};

int main(void *void_args)
{
    icp_thread_setname(("icp_worker_" + std::to_string(rte_lcore_id())).c_str());

    auto args = reinterpret_cast<main_args*>(void_args);

    void *context = args->context;
    std::unique_ptr<void, icp_socket_deleter> control(
        icp_socket_get_client_subscription(context, endpoint.data(), ""));

    auto me = worker(context, control.get(), args->recycler, args->fib);

    /* XXX: args are unavailable after this point */
    icp_task_sync_ping(context, args->endpoint.data());

    while (auto cmd = recv_message(control.get())) {
        me.dispatch(*cmd);
    }

    ICP_LOG(ICP_LOG_WARNING, "queue worker %d exited\n", rte_lcore_id());

    return (0);
}

}
