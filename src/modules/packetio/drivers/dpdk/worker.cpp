#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <zmq.h>
#include "lwip/netif.h"

#include "core/icp_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/epoll_poller.h"
#include "packetio/drivers/dpdk/queue_utils.h"
#include "packetio/drivers/dpdk/worker_api.h"
#include "packetio/memory/dpdk/pbuf_utils.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace worker {

using switch_t = std::shared_ptr<const vif_map<netif>>;

const std::string endpoint = "inproc://icp_packetio_workers_control";

static bool proxy = false;

static constexpr uint16_t pkt_burst_size = 32;
static constexpr int mbuf_prefetch_offset = 4;

using namespace std::chrono_literals;
static constexpr auto poll_timeout = 250ms;

/**
 * Our worker implements a simple finite state machine with
 * API messages as events.
 */
using command_msg = std::variant<start_msg,
                                 stop_msg,
                                 configure_msg>;

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

    const StateVariant& current_state()
    {
        return (m_state);
    }
};

static bool all_pollable(const std::vector<pollable_ptr>& queues)
{
    epoll_poller poller;

    auto add_visitor = [&](auto q) -> bool {
                           return (poller.add(q));
                       };

    auto del_visitor = [&](auto q) {
                           poller.del(q);
                       };

    for (auto& q : queues) {
        if (!std::visit(add_visitor, q)) {
            return (false);
        }
        std::visit(del_visitor, q);
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

static uint16_t rx_burst(const switch_t& vif, const rx_queue* rxq)
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
        auto ifp = vif->find(rxq->port_id(), eth->d_addr.addr_bytes);
        interfaces[i] = ifp ? *ifp : nullptr;
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
        for (auto ifp : vif->find(rxq->port_id())) {
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

static uint16_t service_queue(const switch_t& vif, const pollable_ptr& queue)
{
    return (std::visit(overloaded_visitor(
                           [&](const rx_queue* rxq) -> uint16_t {
                               return (rx_burst(vif, rxq));
                           },
                           [](const tx_queue* txq) -> uint16_t {
                               return (tx_burst(txq));
                           },
                           [](const zmq_socket*) -> uint16_t {
                               return (0);  /* noop; not a queue */
                           }),
                       queue));
}

static void enable_queue_interrupt(const pollable_ptr& queue)
{
    auto enable_visitor = [](auto q) { q->enable(); };
    std::visit(enable_visitor, queue);
}

static void disable_queue_interrupt(const pollable_ptr& queue)
{
    auto disable_visitor = [](auto q) { q->disable(); };
    std::visit(disable_visitor, queue);
}

static void run_pollable(void* control, const switch_t& vif,
                         const std::vector<pollable_ptr>& queues)
{
    epoll_poller poller;

    for (auto& q : queues) {
        poller.add(q);
    }

    auto ctrl_sock = zmq_socket(control);
    poller.add(&ctrl_sock);

    uint16_t max_burst = 0;
    uint16_t idle_queues = 0;

    for (;;) {
        do {
            /* Service queues as fast as possible while packets are present. */
            max_burst = 0;
            idle_queues = 0;

            for (auto& q : queues) {
                auto burst_size = service_queue(vif, q);
                max_burst = std::max(max_burst, burst_size);
                idle_queues += !burst_size;
            }
        } while (max_burst > 0);

        /* All queues are idle; sleep until we have something to do. */
        assert(idle_queues == queues.size());

        for (auto& q : queues) {
            enable_queue_interrupt(q);
        }
        poller.wait_for_interrupt();
        for (auto& q : queues) {
            disable_queue_interrupt(q);
        }

        if (ctrl_sock.readable()) break;
    }

    for (auto& q : queues) {
        poller.del(q);
    }

    poller.del(&ctrl_sock);
}

static void run_spinning(void* control, const switch_t& vif,
                         const std::vector<pollable_ptr>& queues)
{
    while (!icp_socket_has_messages(control)) {
        /*
         * Check the time before servicing any queues.  We want to query our control
         * socket periodically for messages without hammering it.
         */
        using clock = std::chrono::steady_clock;
        auto start = clock::now();

        do {
            uint16_t idle_queues = 0;

            /* Service queues as fast as possible if any of them have packets. */
            do {
                idle_queues = 0;
                for (auto& q : queues) {
                    auto burst_size = service_queue(vif, q);
                    if (!burst_size) idle_queues++;
                }
            } while (idle_queues < queues.size());

            /* All queues are idle; take a break from polling */
            rte_delay_us(1);

            /* Keep doing this until we need to query our control socket */
        } while (clock::now() - start > poll_timeout);
    }
}

static void run(void* control, const switch_t& vif,
                const std::vector<pollable_ptr>& queues)
{
    if (all_pollable(queues)) {
        run_pollable(control, vif, queues);
    } else {
        run_spinning(control, vif, queues);
    }
}

class worker : public finite_state_machine<worker, state, command_msg>
{
    std::vector<pollable_ptr> m_queues;
    void* m_context;
    void* m_control;
    switch_t m_switch;

    void update_config(const std::vector<descriptor>& descriptors)
    {
        m_queues.clear();

        for (auto& d: descriptors) {
            if (d.worker_id != rte_lcore_id()) continue;

            if (d.rxq != nullptr) {
                ICP_LOG(ICP_LOG_DEBUG, "Enabling RX port queue %u:%u on logical core %u\n",
                        d.rxq->port_id(), d.rxq->queue_id(), rte_lcore_id());
                m_queues.emplace_back(d.rxq);
            }

            if (d.txq != nullptr) {
                ICP_LOG(ICP_LOG_DEBUG, "Enabling TX port queue %u:%u on logical core %u\n",
                        d.txq->port_id(), d.txq->queue_id(), rte_lcore_id());
                m_queues.emplace_back(d.txq);
            }
        }
    }

public:
    worker(void *context, void* control, const switch_t& vif)
        : m_context(context)
        , m_control(control)
        , m_switch(vif)
    {}

    void service_queues()
    {
        uint16_t idle_queues = 0;

        do {
            idle_queues = 0;
            for (auto& q : m_queues) {
                auto burst_size = service_queue(m_switch, q);
                if (!burst_size) idle_queues++;
            }
        } while (idle_queues < m_queues.size());
    }

    /* State transition functions */
    std::optional<state> on_event(state_init&, const configure_msg& msg)
    {
        update_config(msg.descriptors);
        return (msg.descriptors.empty()
                ? std::nullopt
                : std::make_optional(state_armed()));
    }

    std::optional<state> on_event(state_armed&, const start_msg &start __attribute__((unused)))
    {
        if (!proxy) {
            icp_task_sync_ping(m_context, start.endpoint.data());
            run(m_control, m_switch, m_queues);
        }
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
        if (!proxy) {
            run(m_control, m_switch, m_queues);
        }
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

static std::optional<command_msg> get_command_msg(void *socket, int flags = 0)
{
    /* who needs classes for RAII? ;-) */
    zmq_msg_t msg __attribute__((cleanup(zmq_msg_close)));

    if (zmq_msg_init(&msg) == -1
        || zmq_msg_recv(&msg, socket, flags) < 0) {
        ICP_LOG(ICP_LOG_ERROR, "Could not receive 0MQ message: %s\n", zmq_strerror(errno));
        return (std::nullopt);
    }

    if (zmq_msg_size(&msg) < sizeof(message)) {
        ICP_LOG(ICP_LOG_ERROR, "Received 0MQ message is too short (%zu < %zu)\n",
                zmq_msg_size(&msg), sizeof(message));
        return (std::nullopt);
    }

    auto m = reinterpret_cast<message*>(zmq_msg_data(&msg));
    switch (m->type) {
    case message_type::CONFIG: {
        std::vector<descriptor> descriptors;
        for (size_t i = 0; i < m->descriptors_size; i++) {
            descriptors.emplace_back(m->descriptors[i]);
        }
        return (std::make_optional(configure_msg { .descriptors = std::move(descriptors) }));
    }
    case message_type::START:
        return (std::make_optional(start_msg { .endpoint = m->endpoint }));
    case message_type::STOP:
        return (std::make_optional(stop_msg { .endpoint = m->endpoint }));
    default:
        return (std::nullopt);
    }
}

int main(void *void_args)
{
    auto args = reinterpret_cast<struct args*>(void_args);

    void *context = args->context;
    std::unique_ptr<void, icp_socket_deleter> control(
        icp_socket_get_client_subscription(context, endpoint.c_str(), ""));

    icp_task_sync_ping(context, args->endpoint.data());

    auto me = worker(context, control.get(), args->vif);

    while (auto cmd = get_command_msg(control.get())) {
        me.dispatch(*cmd);
    }

    ICP_LOG(ICP_LOG_WARNING, "queue worker %d exited\n", rte_lcore_id());

    return (0);
}

static struct {
    void* cntxt;
    std::vector<descriptor> dscrptrs;
    std::shared_ptr<vif_map<netif>> vif;
} proxy_config_data;

void proxy_stash_config_data(const void* cntxt,
                             const std::vector<descriptor>& dscrptrs,
                             const std::shared_ptr<vif_map<netif>>& vif)
{
    proxy_config_data = { .cntxt = const_cast<void*>(cntxt),
                             .dscrptrs = dscrptrs,
                             .vif = vif };
    proxy = true;
}

}
}
}
}

extern "C" {

using namespace icp::packetio::dpdk::worker;

void worker_proxy(bool shutdown)
{
    static void* c;
    static worker* w;
    static auto configured = false;

    if (shutdown) {
        icp_socket_close(c);
        return;
    }

    if (!configured) {
        c = icp_socket_get_client_subscription(proxy_config_data.cntxt, 
                                               endpoint.c_str(), "");
        w = new worker(proxy_config_data.cntxt, c, proxy_config_data.vif);
        auto cmd_configure = std::make_optional(
            configure_msg { .descriptors = proxy_config_data.dscrptrs });
        w->dispatch(*cmd_configure);
        auto cmd_start = std::make_optional(start_msg {});
        w->dispatch(*cmd_start);
        configured = true;
    }

    if (std::holds_alternative<state_running>(w->current_state())) {
        w->service_queues();
        if (icp_socket_has_messages(c)) {
            auto cmd = get_command_msg(c);
            w->dispatch(*cmd);
        }
    }
}

}
