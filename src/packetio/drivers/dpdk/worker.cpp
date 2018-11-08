#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <zmq.h>
#include "lwip/netif.h"

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/queue_poller.h"
#include "packetio/drivers/dpdk/queue_utils.h"
#include "packetio/drivers/dpdk/worker_api.h"
#include "packetio/memory/dpdk/pbuf.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace worker {

using switch_t = std::shared_ptr<const vif_map<netif>>;

const std::string endpoint = "inproc://icp_packetio_workers_control";

static constexpr uint16_t rx_burst_size = 32;

/*
 * Convenience definition for identifying queues, which contain a port
 * and queue id.
*/
using queue_id = std::pair<uint16_t, uint16_t>;

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

static bool all_pollable(const std::vector<queue_id>& queues)
{
    queue_poller poller;
    for (auto& q : queues) {
        if (!poller.add(q.first, q.second)) {
            return (false);
        }
        poller.del(q.first, q.second);
    }
    return (true);
}

static std::pair<size_t, size_t> partition_mbufs(rte_mbuf* incoming[], int length,
                                                 rte_mbuf* unicast[], rte_mbuf* multicast[])
{
    static constexpr int mbuf_prefetch_offset = 4;
    size_t ucast_idx = 0, mcast_idx = 0;
    int i = 0;

    /*
     * Pre-fetching the payload data to the CPU cache is critical for parsing
     * performance.  This series of loops is inteded to keep the cache filled
     * with pending data.
     */

    /* First, prefetch some mbufs. */
    for (i = 0; i < mbuf_prefetch_offset && i < length; i++) {
        rte_prefetch0(incoming[i]);
    }

    /*
     * Now load the payloads from the prefetched mbufs and continue to
     * prefetch new mbufs.
     */
    for (i = 0; i < (2 * mbuf_prefetch_offset) && i < length; i++) {
        rte_prefetch0(incoming[i + mbuf_prefetch_offset]);
        rte_prefetch0(rte_pktmbuf_mtod(incoming[i], void*));
    }

    /*
     * Now we should have mbuf payloads in the local cache, so parse them.
     * Continue prefetching both mbufs and payloads ahead of use.
     */
    for (i = 0; i < (length - (2 * mbuf_prefetch_offset)); i++) {
        rte_prefetch0(incoming[i + (2 * mbuf_prefetch_offset)]);
        rte_prefetch0(rte_pktmbuf_mtod(incoming[i + mbuf_prefetch_offset], void*));
        auto eth = rte_pktmbuf_mtod(incoming[i], struct ether_hdr*);
        if (is_unicast_ether_addr(&eth->d_addr)) {
            unicast[ucast_idx++] = incoming[i];
        } else {
            multicast[mcast_idx++] = incoming[i];
        }
    }

     /*
     * All mbufs have been prefetched.  Continue to prefetch payloads while
     * parsing the remaining mbufs.
     */
    for (; i < length - mbuf_prefetch_offset; i++) {
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

static void rx_burst(const switch_t& vif, const queue_id& q)
{
    /*
     * We pull packets from the port and put them in the receive array.
     * We then sort them into unicast and multicast types.
     */
    std::array<rte_mbuf*, rx_burst_size> incoming, unicast, nunicast;
    std::array<netif*, rx_burst_size> interfaces;

    while (auto n = rte_eth_rx_burst(q.first, q.second, incoming.data(), rx_burst_size)) {
        if (n) {
            ICP_LOG(ICP_LOG_TRACE, "Received %d packet%s on %d:%d\n",
                    n, n > 1 ? "s" : "", q.first, q.second);
        }

        auto lengths = partition_mbufs(incoming.data(), n,
                                       unicast.data(), nunicast.data());

        /* Lookup interfaces for unicast packets ... */
        for (size_t i = 0; i < lengths.first; i++) {
            auto eth = rte_pktmbuf_mtod(unicast[i], struct ether_hdr *);
            auto ifp = vif->find(q.first, eth->d_addr.addr_bytes);
            interfaces[i] = ifp ? *ifp : nullptr;
        }

        /* ... and dispatch */
        for (size_t i = 0; i < lengths.first; i++) {
            if (interfaces[i]) {
                ICP_LOG(ICP_LOG_TRACE, "Dispatching unicast packet to %c%c%u\n",
                        interfaces[i]->name[0], interfaces[i]->name[1], interfaces[i]->num);
                interfaces[i]->input(packetio_pbuf_synchronize(unicast[i]),
                                     interfaces[i]);
            } else {
                rte_pktmbuf_free(unicast[i]);
            }
        }

        for (size_t i = 0; i < lengths.second; i++) {
            auto pbuf = packetio_pbuf_synchronize(nunicast[i]);
            for (auto ifp : vif->find(q.first)) {
                ICP_LOG(ICP_LOG_TRACE, "Dispatching non-unicast packet to %c%c%u\n",
                        ifp->name[0], ifp->name[1], ifp->num);
                rte_pktmbuf_refcnt_update(nunicast[i], 1);
                ifp->input(pbuf, ifp);
            }
            rte_pktmbuf_free(nunicast[i]);
        }
    }
}

static void run_pollable(void* control, const switch_t& vif,
                         const std::vector<queue_id>& queues)
{
    queue_poller poller;
    for (auto& q : queues) {
        poller.add(q.first, q.second);
        poller.enable(q.first, q.second);
    }

    do {
        for (auto& q : poller.poll(250)) {
            rx_burst(vif, q);
            poller.enable(q.first, q.second);
        }
    } while (!icp_socket_has_messages(control));
}

static void run_spinning(void* control, const switch_t& vif,
                         const std::vector<queue_id>& queues)
{
    do {
        for (auto& q : queues) {
            rx_burst(vif, q);
        }
    } while (!icp_socket_has_messages(control));
}

static void run(void* control, const switch_t& vif,
                const std::vector<queue_id>& queues)
{
    if (all_pollable(queues)) {
        run_pollable(control, vif, queues);
    } else {
        run_spinning(control, vif, queues);
    }
}

class worker : public finite_state_machine<worker, state, command_msg>
{
    std::vector<queue_id> m_rxqs;
    std::vector<queue_id> m_txqs;

    void* m_context;
    void* m_control;
    switch_t m_switch;

    void update_config(const std::vector<queue::descriptor>& descriptors)
    {
        m_rxqs.clear();
        m_txqs.clear();

        for (auto& d: descriptors) {
            if (d.worker_id == rte_lcore_id()) {
                if (d.mode == queue::queue_mode::RX
                    || d.mode == queue::queue_mode::RXTX) {
                    ICP_LOG(ICP_LOG_TRACE, "Enabling RX port queue %d:%d\n",
                            d.port_id, d.queue_id);
                    m_rxqs.emplace_back(d.port_id, d.queue_id);
                }
                if (d.mode == queue::queue_mode::TX
                    || d.mode == queue::queue_mode::RXTX) {
                    m_txqs.emplace_back(d.port_id, d.queue_id);
                    ICP_LOG(ICP_LOG_TRACE, "Enabling TX port queue %d:%d\n",
                            d.port_id, d.queue_id);
                }
            }
        }
    }

public:
    worker(void *context, void* control, const switch_t& vif)
        : m_context(context)
        , m_control(control)
        , m_switch(vif)
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
        run(m_control, m_switch, m_rxqs);
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
        run(m_control, m_switch, m_rxqs);
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
        std::vector<queue::descriptor> descriptors;
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

}
}
}
}
