#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <zmq.h>
#include "lwip/netif.h"
#include "lwip/snmp.h"

#include "core/op_log.h"
#include "core/op_thread.h"
#include "utils/prefetch_for_each.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/mbuf_tx.hpp"
#include "packetio/drivers/dpdk/queue_utils.hpp"
#include "packetio/memory/dpdk/pbuf_utils.h"
#include "packetio/workers/dpdk/callback.hpp"
#include "packetio/workers/dpdk/epoll_poller.hpp"
#include "packetio/workers/dpdk/event_loop_adapter.hpp"
#include "packetio/workers/dpdk/rx_queue.hpp"
#include "packetio/workers/dpdk/tx_queue.hpp"
#include "packetio/workers/dpdk/tx_scheduler.hpp"
#include "packetio/workers/dpdk/zmq_socket.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"
#include "timesync/chrono.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packetio::dpdk::worker {

const std::string_view endpoint = "inproc://op_packetio_workers_control";

static constexpr int mbuf_prefetch_offset = 8;

static const rte_gro_param gro_params = {.gro_types = RTE_GRO_TCP_IPV4,
                                         .max_flow_num = pkt_burst_size,
                                         .max_item_per_flow = pkt_burst_size};

/**
 * We only have two states we transition between, based on our messages:
 * stopped and started.  We use each struct as a tag for each state.  And
 * we wrap them all in std::variant for ease of use.
 */
struct state_stopped
{};
struct state_started
{};

using state = std::variant<state_stopped, state_started>;

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
        auto& child = static_cast<Derived&>(*this);
        try {
            auto next_state = std::visit(
                [&](auto& state,
                    const auto& event) -> std::optional<StateVariant> {
                    return (child.on_event(state, event));
                },
                m_state,
                event);

            if (next_state) { m_state = *next_state; }
        } catch (const std::exception& e) {
            OP_LOG(OP_LOG_ERROR, "%s\n", e.what());
        }
    }
};

static bool all_pollable(const std::vector<task_ptr>& tasks)
{
    epoll_poller poller;

    auto add_visitor = [&](auto task) -> bool { return (poller.add(task)); };

    auto del_visitor = [&](auto task) { poller.del(task); };

    for (auto& task : tasks) {
        if (!std::visit(add_visitor, task)) { return (false); }
        std::visit(del_visitor, task);
    }
    return (true);
}

/**
 * Set the interface pointer the mbuf will be dispatched to.
 *
 * This interface pointer is only valid for unicast packets during parts of the
 * rx dispatch.
 */
static void rx_mbuf_set_ifp(rte_mbuf* mbuf, netif* ifp)
{
    mbuf->userdata = ifp;
}

/**
 * Get the interface pointer the mbuf will be dispatchd to.
 *
 * This interface pointer is only valid for unicast packets during parts of the
 * rx dispatch.
 */
netif* rx_mbuf_get_ifp(rte_mbuf* mbuf)
{
    return reinterpret_cast<netif*>(mbuf->userdata);
}

/**
 * Resolve interfaces for the packets and store interface in the packet
 * ancillary data.  If the mbuf is destined to an interface which is not found,
 * the mbuf is added to the list of packets to free.
 */
static std::pair<uint16_t, uint16_t> rx_resolve_interfaces(const fib* fib,
                                                           const rx_queue* rxq,
                                                           rte_mbuf* incoming[],
                                                           uint16_t n,
                                                           rte_mbuf* to_stack[],
                                                           rte_mbuf* to_free[])
{
    uint16_t nb_to_stack = 0, nb_to_free = 0;
    auto port_id = rxq->port_id();

    /*
     * Pre-fetching the payload data to the CPU cache is critical for parsing
     * performance.  This series of loops is inteded to keep the cache filled
     * with pending data.
     */

    utils::prefetch_for_each(
        incoming,
        incoming + n,
        [](auto mbuf) {
            /* prefetch mbuf payload */
            rte_prefetch0(rte_pktmbuf_mtod(mbuf, void*));
        },
        [&](auto mbuf) {
            auto eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
            if (rte_is_unicast_ether_addr(&eth->d_addr)) {
                auto ifp = fib->find_interface(port_id, eth->d_addr.addr_bytes);
                rx_mbuf_set_ifp(mbuf, ifp);
                if (ifp) {
                    to_stack[nb_to_stack++] = mbuf;
                } else {
                    // Packet doesn't match any interfaces
                    to_free[nb_to_free++] = mbuf;
                }
            } else {
                to_stack[nb_to_stack++] = mbuf;
                rx_mbuf_set_ifp(mbuf, nullptr);
            }
        },
        mbuf_prefetch_offset);

    return std::make_pair(nb_to_stack, nb_to_free);
}

/**
 * Dispatch unicast packets to sinks on the interface.
 */
static void rx_interface_sink_push_unicast_burst(
    netif* ifp,
    const std::vector<packet::generic_sink>& sinks,
    rte_mbuf* incoming[],
    uint16_t n)
{
    for (auto& sink : sinks) {
        if (!sink.active()) { continue; }

        OP_LOG(OP_LOG_TRACE,
               "Dispatching packets to interface %c%c%u sink %s\n",
               ifp->name[0],
               ifp->name[1],
               ifp->num,
               sink.id().c_str());
        sink.push(reinterpret_cast<packet::packet_buffer* const*>(incoming), n);
    }
}

/**
 * Dispatch multicast packets to all interface sinks on the port.
 *
 * This function assumes all packets in the incoming burst are
 * multicast packets.
 */
static void rx_interface_sink_push_multicast_burst(const fib* fib,
                                                   const rx_queue* rxq,
                                                   rte_mbuf* incoming[],
                                                   uint16_t n)
{
    // Multicast dispatch to all sinks on port
    // This will be slow when there are a lot of interfaces
    for (auto&& [idx, entry] : fib->get_interfaces(rxq->port_id())) {
        auto ifp = entry->ifp;
        for (auto& sink : entry->rx_sinks) {
            if (sink.active()) {
                OP_LOG(OP_LOG_TRACE,
                       "Dispatching packets to interface %c%c%u sink %s\n",
                       ifp->name[0],
                       ifp->name[1],
                       ifp->num,
                       sink.id().c_str());
                sink.push(
                    reinterpret_cast<packet::packet_buffer* const*>(incoming),
                    n);
            }
        }
    }
}

/**
 * Dispatch packets to interface sinks on the port.
 *
 * Packet order is preserved and contiguous bursts to the same
 * sink are pased as a burst.
 */
static std::pair<uint16_t, uint16_t>
rx_interface_sink_dispatch(const fib* fib,
                           const rx_queue* rxq,
                           rte_mbuf* incoming[],
                           uint16_t n,
                           rte_mbuf* to_stack[],
                           rte_mbuf* to_free[])
{
    constexpr int START = 0, COUNT = 1, IFP_SINKS = 2;
    using burst_tuple =
        std::tuple<uint16_t, uint16_t, const worker::fib::interface_sinks*>;
    std::array<burst_tuple, pkt_burst_size> bursts;
    burst_tuple* burst = nullptr;
    uint16_t nbursts = 0;
    uint16_t nb_to_stack = 0, nb_to_free = 0;

    rte_ether_addr* last_dst_addr = nullptr;
    const worker::fib::interface_sinks* last_entry = nullptr;

    /*
     * Pre-fetching the payload data to the CPU cache is critical for parsing
     * performance.  This series of loops is inteded to keep the cache filled
     * with pending data.
     */

    utils::prefetch_for_each(
        incoming,
        incoming + n,
        [](auto mbuf) {
            /* prefetch mbuf payload */
            rte_prefetch0(rte_pktmbuf_mtod(mbuf, void*));
        },
        [&](auto mbuf) {
            auto eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);
            auto dst_addr = &eth->d_addr;
            bool unicast = rte_is_unicast_ether_addr(dst_addr);

            if (unicast) {
                // Unicast packets must match an interface or they will be
                // discarded
                if (!last_dst_addr
                    || !rte_is_same_ether_addr(dst_addr, last_dst_addr)) {
                    last_dst_addr = dst_addr;
                    last_entry = fib->find_interface_and_sinks(
                        rxq->port_id(), dst_addr->addr_bytes);
                }
                if (!last_entry) {
                    // Packet doesn't match any interfaces
                    to_free[nb_to_free++] = mbuf;
                    return;
                } else {
                    // Store interface pointers to avoid multiple lookups for
                    // the same packet
                    rx_mbuf_set_ifp(mbuf, last_entry->ifp);
                    to_stack[nb_to_stack++] = mbuf;
                }
            } else {
                // Multicast packets don't need to match anything
                // They will be delivered to all sinks on the port.
                to_stack[nb_to_stack++] = mbuf;
                rx_mbuf_set_ifp(mbuf, nullptr);
            }
            if (!burst || std::get<IFP_SINKS>(*burst) != last_entry) {
                // Start a new burst
                burst = &bursts[nbursts++];
                *burst = {nb_to_stack - 1, 1, last_entry};
            } else {
                // Extend the current burst
                ++std::get<COUNT>(*burst);
            }
        },
        mbuf_prefetch_offset);

    std::for_each(bursts.data(), bursts.data() + nbursts, [&](auto& tuple) {
        auto entry = std::get<IFP_SINKS>(tuple);
        if (entry) {
            rx_interface_sink_push_unicast_burst(entry->ifp,
                                                 entry->rx_sinks,
                                                 to_stack
                                                     + std::get<START>(tuple),
                                                 std::get<COUNT>(tuple));
        } else {
            rx_interface_sink_push_multicast_burst(fib,
                                                   rxq,
                                                   to_stack
                                                       + std::get<START>(tuple),
                                                   std::get<COUNT>(tuple));
        }
    });

    return std::make_pair(nb_to_stack, nb_to_free);
}

static void rx_stack_dispatch(const fib* fib,
                              const rx_queue* rxq,
                              rte_mbuf* incoming[],
                              uint16_t n)
{
    std::array<rte_mbuf*, pkt_burst_size> unicast, nunicast;

    // Split up the unicast and non-unicast packets
    auto [unicast_last, nunicast_last] = std::partition_copy(
        incoming, incoming + n, unicast.data(), nunicast.data(), [](auto mbuf) {
            // Unicast packets should have a non null interace
            return rx_mbuf_get_ifp(mbuf);
        });

    // Dispatch unicast packets
    std::for_each(unicast.data(), unicast_last, [](auto mbuf) {
        auto ifp = rx_mbuf_get_ifp(mbuf);
        rx_mbuf_set_ifp(mbuf, nullptr); // Clear just to be safe

        OP_LOG(OP_LOG_TRACE,
               "Dispatching unicast packet to %c%c%u\n",
               ifp->name[0],
               ifp->name[1],
               ifp->num);

        if (ifp->input(packetio_memory_pbuf_synchronize(mbuf), ifp) != ERR_OK) {
            MIB2_STATS_NETIF_INC_ATOMIC(ifp, ifindiscards);
            rte_pktmbuf_free(mbuf);
        }
    });

    /*
     * Dispatch all non-unicast packets to all interfaces.
     * To do this efficiently we make a clone of each mbuf, which makes
     * a copy of the packet metadata with a pointer to the original
     * packet buffer.  This allows each receiving interface to adjust
     * their payload pointers as necessary while parsing the headers.
     */
    std::for_each(nunicast.data(), nunicast_last, [&](auto mbuf) {
        for (auto&& [idx, entry] : fib->get_interfaces(rxq->port_id())) {
            auto ifp = entry->ifp;

            OP_LOG(OP_LOG_TRACE,
                   "Dispatching non-unicast packet to %c%c%u\n",
                   ifp->name[0],
                   ifp->name[1],
                   ifp->num);

            auto clone = rte_pktmbuf_clone(mbuf, mbuf->pool);
            if (!clone
                || ifp->input(packetio_memory_pbuf_synchronize(clone), ifp)
                       != ERR_OK) {
                MIB2_STATS_NETIF_INC_ATOMIC(ifp, ifindiscards);
                rte_pktmbuf_free(
                    clone); /* Note: this free handles null correctly */
            }
        }
        rte_pktmbuf_free(mbuf);
    });
}

static void rx_interface_dispatch(const fib* fib,
                                  const rx_queue* rxq,
                                  rte_mbuf* incoming[],
                                  uint16_t n)
{
    uint16_t nb_to_free = 0, nb_to_stack = 0;
    std::array<rte_mbuf*, pkt_burst_size> to_stack, to_free;
    auto has_hardware_tags = (rxq->flags() & rx_feature_flags::hardware_tags);

    if (fib->has_interface_rx_sinks(rxq->port_id())) {
        // Dispatch packets to interface sinks and lookup interfaces
        auto [nb_resolved, nb_unresolved] = rx_interface_sink_dispatch(
            fib, rxq, incoming, n, to_stack.data(), to_free.data());
        nb_to_stack = nb_resolved;
        nb_to_free += nb_unresolved;

        auto [to_stack_last, to_free_last] =
            std::partition_copy(to_stack.data(),
                                to_stack.data() + nb_to_stack,
                                to_stack.data(),
                                to_free.data() + nb_to_free,
                                [&](auto mbuf) {
                                    // Filter signature packets from being
                                    // processed by the stack
                                    if (mbuf_signature_avail(mbuf))
                                        return false;
                                    if (has_hardware_tags) {
                                        // Filter packets with hardware tags
                                        if (!(mbuf->ol_flags & PKT_RX_FDIR)
                                            || !mbuf->hash.fdir.hi) {
                                            return false;
                                        }
                                    }
                                    return true;
                                });
        nb_to_stack = std::distance(to_stack.data(), to_stack_last);
        nb_to_free = std::distance(to_free.data(), to_free_last);

    } else {
        auto [to_stack_last, to_free_last] =
            std::partition_copy(incoming,
                                incoming + n,
                                to_stack.data(),
                                to_free.data(),
                                [&](auto mbuf) {
                                    // Filter signature packets from being
                                    // processed by the stack
                                    if (mbuf_signature_avail(mbuf))
                                        return false;
                                    if (has_hardware_tags) {
                                        // Filter packets with hardware tags
                                        if (!(mbuf->ol_flags & PKT_RX_FDIR)
                                            || !mbuf->hash.fdir.hi) {
                                            return false;
                                        }
                                    }
                                    return true;
                                });
        nb_to_stack = std::distance(to_stack.data(), to_stack_last);
        nb_to_free = std::distance(to_free.data(), to_free_last);

        // Resolve interface for packets
        auto [nb_resolved, nb_unresolved] =
            rx_resolve_interfaces(fib,
                                  rxq,
                                  to_stack.data(),
                                  nb_to_stack,
                                  to_stack.data(),
                                  to_free.data() + nb_to_free);
        nb_to_stack = nb_resolved;
        nb_to_free += nb_unresolved;
    }

    /* Hand any stack packets off to the stack... */
    if (nb_to_stack) {
        /*
         * If we don't have hardware LRO support, try to coalesce some
         * packets before handing them up the stack.
         */
        if (!(rxq->flags() & rx_feature_flags::hardware_lro)) {
            nb_to_stack = rte_gro_reassemble_burst(
                to_stack.data(), nb_to_stack, &gro_params);
        }

        rx_stack_dispatch(fib, rxq, to_stack.data(), nb_to_stack);
    }

    /* ... and free all the non-stack packets */
    std::for_each(to_free.data(), to_free.data() + nb_to_free, [](auto mbuf) {
        rx_mbuf_set_ifp(mbuf, nullptr); // Clear just to be safe
        rte_pktmbuf_free(mbuf);
    });
}

static void rx_sink_dispatch(const fib* fib,
                             const rx_queue* rxq,
                             rte_mbuf* const incoming[],
                             uint16_t n)
{
    for (auto& sink : fib->get_rx_sinks(rxq->port_id())) {
        if (!sink.active()) { continue; }

        OP_LOG(OP_LOG_TRACE,
               "Dispatching packets to sink %s\n",
               sink.id().c_str());
        sink.push(reinterpret_cast<packet::packet_buffer* const*>(incoming), n);
    }
}

static uint16_t rx_burst(const fib* fib, const rx_queue* rxq)
{
    std::array<rte_mbuf*, pkt_burst_size> incoming;

    auto n = rte_eth_rx_burst(
        rxq->port_id(), rxq->queue_id(), incoming.data(), pkt_burst_size);

    if (!n) return (0);

    OP_LOG(OP_LOG_TRACE,
           "Received %d packet%s on %d:%d\n",
           n,
           n > 1 ? "s" : "",
           rxq->port_id(),
           rxq->queue_id());

    /* Clear direction flags */
    std::for_each(incoming.data(), incoming.data() + n, [](auto mbuf) {
        mbuf_tx_clear(mbuf);
    });

    /* Dispatch packets to any port sinks */
    rx_sink_dispatch(fib, rxq, incoming.data(), n);

    /* Dispatch packets to all interface level sinks and interfaces */
    rx_interface_dispatch(fib, rxq, incoming.data(), n);

    return (n);
}

static void tx_interface_sink_dispatch(const fib* fib,
                                       uint16_t port_id,
                                       rte_mbuf* const outgoing[],
                                       uint16_t n)
{
    constexpr int START = 0, COUNT = 1, SINKS = 2;
    using burst_tuple = std::tuple<uint16_t,
                                   uint16_t,
                                   decltype(fib->find_interface_tx_sinks(
                                       0, libpacket::type::mac_address{}))>;
    std::array<burst_tuple, pkt_burst_size> bursts;
    burst_tuple* burst = nullptr;
    uint16_t nbursts = 0;

    rte_ether_addr burst_hw_addr{};
    uint16_t processed = 0;

    // Create bursts of contiguous packets to the same interface
    std::for_each(outgoing, outgoing + n, [&](auto mbuf) {
        rte_ether_addr hw_addr;
        mbuf_tx_get_hwaddr(mbuf, hw_addr.addr_bytes);
        if (!rte_is_same_ether_addr(&hw_addr, &burst_hw_addr)) {
            auto sinks =
                fib->find_interface_tx_sinks(port_id, hw_addr.addr_bytes);
            if (sinks) {
                // Start a new burst
                assert(nbursts < bursts.size());
                burst = &bursts[nbursts++];
                *burst = {processed, 1, sinks};
            } else {
                // No sinks found, so can skip this burst
                burst = nullptr;
            }
            burst_hw_addr = hw_addr;
        } else {
            // Extend the current burst
            if (burst) { ++std::get<COUNT>(*burst); }
        }
        ++processed;
    });

    // Push the bursts to the sinks
    std::for_each(bursts.data(), bursts.data() + nbursts, [&](auto& burst) {
        auto& sinks = std::get<SINKS>(burst);
        std::for_each(std::begin(*sinks), std::end(*sinks), [&](auto& sink) {
            if (!sink.active()) return;
            sink.push(reinterpret_cast<packet::packet_buffer* const*>(outgoing)
                          + std::get<START>(burst),
                      std::get<COUNT>(burst));
        });
    });
}

static void tx_timestamp_packets(rte_mbuf* packets[], uint16_t n)
{
    using clock = openperf::timesync::chrono::realtime;
    auto now = clock::now().time_since_epoch();
    auto timestamp =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    std::for_each(packets, packets + n, [timestamp](auto m) {
        m->ol_flags |= PKT_RX_TIMESTAMP;
        m->timestamp = timestamp;
    });
}

void tx_sink_dispatch(const fib* fib,
                      uint16_t port_id,
                      rte_mbuf* outgoing[],
                      uint16_t n)
{
    // Dispatch to port level Tx sinks
    auto& port_sinks = fib->get_tx_sinks(port_id);
    auto interface_sinks = fib->has_interface_tx_sinks(port_id);

    if (port_sinks.empty() && !interface_sinks) return;

    tx_timestamp_packets(outgoing, n);

    for (auto& sink : port_sinks) {
        if (!sink.active()) { continue; }
        sink.push(reinterpret_cast<packet::packet_buffer* const*>(outgoing), n);
    }

    // Dispatch to interface level Tx sinks
    if (interface_sinks) {
        tx_interface_sink_dispatch(fib, port_id, outgoing, n);
    }
}

static uint16_t tx_burst(const fib* fib, const tx_queue* txq)
{
    std::array<rte_mbuf*, pkt_burst_size> outgoing;

    auto to_send =
        rte_ring_dequeue_burst(txq->ring(),
                               reinterpret_cast<void**>(outgoing.data()),
                               outgoing.size(),
                               nullptr);

    if (!to_send) return (0);

    // Need to do the Tx sink dispatch before sending the packets
    // The driver could possibly free the mbuf or when packets are
    // sent using a ring driver, modify the packet data before
    // the packet gets pushed to the sink.
    tx_sink_dispatch(fib, txq->port_id(), outgoing.data(), to_send);

    auto sent = rte_eth_tx_burst(
        txq->port_id(), txq->queue_id(), outgoing.data(), to_send);

    OP_LOG(OP_LOG_TRACE,
           "Transmitted %u of %u packet%s on %u:%u\n",
           sent,
           to_send,
           sent > 1 ? "s" : "",
           txq->port_id(),
           txq->queue_id());

    size_t retries = 0;
    while (sent < to_send) {
        rte_pause();
        retries++;
        sent += rte_eth_tx_burst(txq->port_id(),
                                 txq->queue_id(),
                                 outgoing.data() + sent,
                                 to_send - sent);
    }

    if (retries) {
        OP_LOG(OP_LOG_DEBUG,
               "Transmission required %zu retries on %u:%u\n",
               retries,
               txq->port_id(),
               txq->queue_id());
    }

    return (sent);
}

static uint16_t service_event(event_loop::generic_event_loop& loop,
                              const fib* fib,
                              const task_ptr& task)
{
    return (std::visit(utils::overloaded_visitor(
                           [&](const callback* cb) -> uint16_t {
                               const_cast<callback*>(cb)->run_callback(loop);
                               return (0);
                           },
                           [&](const rx_queue* rxq) -> uint16_t {
                               return (rx_burst(fib, rxq));
                           },
                           [&](const tx_queue* txq) -> uint16_t {
                               auto nb_pkts = tx_burst(fib, txq);
                               const_cast<tx_queue*>(txq)->enable();
                               return (nb_pkts);
                           },
                           [](const tx_scheduler* scheduler) -> uint16_t {
                               const_cast<tx_scheduler*>(scheduler)->run();
                               return (0);
                           },
                           [](const zmq_socket*) -> uint16_t {
                               return (0); /* noop */
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
struct has_enable : std::false_type
{};

template <typename T>
struct has_enable<T, std::void_t<decltype(std::declval<T>().enable())>>
    : std::true_type
{};

static void enable_event_interrupt(const task_ptr& task)
{
    auto enable_visitor = [](auto t) {
        if constexpr (has_enable<decltype(*t)>::value) { t->enable(); }
    };
    std::visit(enable_visitor, task);
}

template <typename, typename = std::void_t<>>
struct has_disable : std::false_type
{};

template <typename T>
struct has_disable<T, std::void_t<decltype(std::declval<T>().disable())>>
    : std::true_type
{};

static void disable_event_interrupt(const task_ptr& task)
{
    auto disable_visitor = [](auto t) {
        if constexpr (has_disable<decltype(*t)>::value) { t->disable(); }
    };
    std::visit(disable_visitor, task);
}

struct run_args
{
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
    auto& loop_adapter = args.loop.get<event_loop_adapter>();
    auto poller = epoll_poller();

    poller.add(&ctrl_sock);

    for (auto& q : args.rx_queues) { poller.add(q); }

    for (auto& p : args.pollables) { poller.add(p); }

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
            auto guard = utils::recycle::guard(*args.recycler, rte_lcore_id());
            for (auto& event : events) {
                std::visit(
                    utils::overloaded_visitor(
                        [&](callback* cb) { cb->run_callback(args.loop); },
                        [&](rx_queue* rxq) {
                            do {
                                while (rx_burst(args.fib, rxq)
                                       == pkt_burst_size) {
                                    rte_pause();
                                }
                                rxq->enable();
                            } while (rx_burst(args.fib, rxq));
                        },
                        [&](tx_queue* txq) {
                            while (tx_burst(args.fib, txq) == pkt_burst_size) {
                                rte_pause();
                            }
                            txq->enable();
                        },
                        [](tx_scheduler* scheduler) { scheduler->run(); },
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

    for (auto& p : args.pollables) { poller.del(p); }

    poller.del(&ctrl_sock);
}

static void run_spinning(run_args&& args)
{
    bool messages = false;
    auto ctrl_sock = zmq_socket(args.control, &messages);
    auto& loop_adapter = args.loop.get<event_loop_adapter>();
    auto poller = epoll_poller();

    poller.add(&ctrl_sock);

    for (auto& p : args.pollables) { poller.add(p); }

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

        rte_pause();

        /* All queues are idle; check callbacks */
        for (auto& event : poller.poll(0)) {
            service_event(args.loop, args.fib, event);
        }

        /* Perform all loop updates before exiting or restarting the loop. */
        loop_adapter.update_poller(poller);
    }

    for (auto& p : args.pollables) { poller.del(p); }

    poller.del(&ctrl_sock);
}

static void run(run_args&& args)
{
    /*
     * XXX: Our control socket is edge triggered, so make sure we drain
     * all of the control messages before jumping into our real run
     * function.  We won't receive any more control interrupts otherwise.
     */
    if (op_socket_has_messages(args.control)) return;

    if (args.rx_queues.empty() || all_pollable(args.rx_queues)) {
        run_pollable(std::forward<run_args>(args));
    } else {
        run_spinning(std::forward<run_args>(args));
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
        for (auto& d : descriptors) {
            if (d.worker_id != rte_lcore_id()) continue;

            std::visit(
                utils::overloaded_visitor(
                    [&](callback* callback) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Adding task %.*s to worker %u\n",
                               static_cast<int>(callback->name().length()),
                               callback->name().data(),
                               rte_lcore_id());
                        m_pollables.emplace_back(callback);
                    },
                    [&](rx_queue* rxq) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Adding RX port queue %u:%u to worker %u\n",
                               rxq->port_id(),
                               rxq->queue_id(),
                               rte_lcore_id());
                        m_rx_queues.emplace_back(rxq);
                    },
                    [&](tx_queue* txq) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Adding TX port queue %u:%u to worker %u\n",
                               txq->port_id(),
                               txq->queue_id(),
                               rte_lcore_id());
                        m_pollables.emplace_back(txq);
                    },
                    [&](tx_scheduler* scheduler) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Adding TX port scheduler %u:%u to worker %u\n",
                               scheduler->port_id(),
                               scheduler->queue_id(),
                               rte_lcore_id());
                        m_pollables.emplace_back(scheduler);
                    }),
                d.ptr);
        }
    }

    void del_config(const std::vector<descriptor>& descriptors)
    {
        for (auto& d : descriptors) {
            if (d.worker_id != rte_lcore_id()) continue;

            /*
             * XXX: Careful!  These pointers have likely been deleted by their
             * owners at this point, so don't dereference them!
             */
            std::visit(
                utils::overloaded_visitor(
                    [&](callback* cb) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Removing task from worker %u\n",
                               rte_lcore_id());
                        m_pollables.erase(std::remove(std::begin(m_pollables),
                                                      std::end(m_pollables),
                                                      task_ptr{cb}),
                                          std::end(m_pollables));
                    },
                    [&](rx_queue* rxq) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Removing RX port queue from worker %u\n",
                               rte_lcore_id());
                        m_rx_queues.erase(std::remove(std::begin(m_rx_queues),
                                                      std::end(m_rx_queues),
                                                      task_ptr{rxq}),
                                          std::end(m_rx_queues));
                    },
                    [&](tx_queue* txq) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Removing TX port queue from worker %u\n",
                               rte_lcore_id());
                        m_pollables.erase(std::remove(std::begin(m_pollables),
                                                      std::end(m_pollables),
                                                      task_ptr{txq}),
                                          std::end(m_pollables));
                    },
                    [&](tx_scheduler* scheduler) {
                        OP_LOG(OP_LOG_DEBUG,
                               "Removing TX port scheduler from worker %u\n",
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
        return (run_args{.control = m_control,
                         .recycler = m_recycler,
                         .loop = m_loop,
                         .fib = m_fib,
                         .rx_queues = m_rx_queues,
                         .pollables = m_pollables});
    }

public:
    worker(void* context, void* control, recycler* recycler, const fib* fib)
        : m_context(context)
        , m_control(control)
        , m_recycler(recycler)
        , m_fib(fib)
        , m_loop(event_loop_adapter{})
    {}

    /* State transition functions */
    std::optional<state> on_event(state_stopped&,
                                  const add_descriptors_msg& add)
    {
        add_config(add.descriptors);
        return (std::nullopt);
    }

    std::optional<state> on_event(state_stopped&,
                                  const del_descriptors_msg& del)
    {
        del_config(del.descriptors);
        return (std::nullopt);
    }

    std::optional<state> on_event(state_started&,
                                  const add_descriptors_msg& add)
    {
        add_config(add.descriptors);

        run(make_run_args());
        return (std::nullopt);
    }

    std::optional<state> on_event(state_started&,
                                  const del_descriptors_msg& del)
    {
        del_config(del.descriptors);
        run(make_run_args());
        return (std::nullopt);
    }

    /* Generic state transition functions */
    template <typename State>
    std::optional<state> on_event(State&, const start_msg& start)
    {
        op_task_sync_ping(m_context, start.endpoint.data());
        run(make_run_args());
        return (std::make_optional(state_started{}));
    }

    template <typename State>
    std::optional<state> on_event(State&, const stop_msg& stop)
    {
        op_task_sync_ping(m_context, stop.endpoint.data());
        return (std::make_optional(state_stopped{}));
    }
};

int main(void* void_args)
{
    op_thread_setname(("op_worker_" + std::to_string(rte_lcore_id())).c_str());

    auto args = reinterpret_cast<main_args*>(void_args);

    void* context = args->context;
    std::unique_ptr<void, op_socket_deleter> control(
        op_socket_get_client_subscription(context, endpoint.data(), ""));

    auto me = worker(context, control.get(), args->recycler, args->fib);

    /* XXX: args are unavailable after this point */
    op_task_sync_ping(context, args->endpoint.data());

    while (auto cmd = recv_message(control.get())) { me.dispatch(*cmd); }

    OP_LOG(OP_LOG_WARNING, "queue worker %d exited\n", rte_lcore_id());

    return (0);
}

} // namespace openperf::packetio::dpdk::worker
