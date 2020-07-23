#ifndef _OP_PACKETIO_DPDK_WORKER_API_HPP_
#define _OP_PACKETIO_DPDK_WORKER_API_HPP_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "core/op_core.h"
#include "lwip/netif.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/forwarding_table.hpp"
#include "packetio/generic_sink.hpp"
#include "packetio/transmit_table.hpp"
#include "packetio/workers/dpdk/tx_source.hpp"
#include "utils/recycle.hpp"

namespace openperf::packetio::dpdk {
class callback;
class rx_queue;
class tx_queue;
class tx_scheduler;
class zmq_socket;

namespace worker {

static constexpr uint16_t pkt_burst_size = 64;

using fib =
    packetio::forwarding_table<netif, packet::generic_sink, RTE_MAX_ETHPORTS>;
using tib = packetio::transmit_table<dpdk::tx_source>;
using recycler = utils::recycle::depot<RTE_MAX_LCORE>;

/**
 * The types of things workers know how to deal with.
 */
using task_ptr =
    std::variant<callback*, rx_queue*, tx_queue*, tx_scheduler*, zmq_socket*>;

/**
 * The types of things we insert into descriptors.
 */
using descriptor_ptr =
    std::variant<callback*, rx_queue*, tx_queue*, tx_scheduler*>;

struct descriptor
{
    uint16_t worker_id;
    descriptor_ptr ptr;

    descriptor(uint16_t id, descriptor_ptr p)
        : worker_id(id)
        , ptr(p)
    {}
};

struct start_msg
{
    std::string endpoint;
};

struct stop_msg
{
    std::string endpoint;
};

struct add_descriptors_msg
{
    std::vector<worker::descriptor> descriptors;
};

struct del_descriptors_msg
{
    std::vector<worker::descriptor> descriptors;
};

/**
 * Our worker is a finite state machine and these messages are
 * the events that trigger state changes.
 */
using command_msg =
    std::variant<start_msg, stop_msg, add_descriptors_msg, del_descriptors_msg>;

extern const std::string_view endpoint;

class client
{
public:
    client(void* context);

    void start(void* context, unsigned nb_workers);
    void stop(void* context, unsigned nb_workers);
    void add_descriptors(const std::vector<worker::descriptor>&);
    void del_descriptors(const std::vector<worker::descriptor>&);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

struct main_args
{
    void* context;
    std::string_view endpoint;
    recycler* recycler;
    const fib* fib;
};

int main(void*);

int send_message(void* socket, const command_msg& msg);
std::optional<command_msg> recv_message(void* socket, int flags = 0);

void tx_sink_dispatch(const fib* fib,
                      uint16_t port_id,
                      rte_mbuf* const outgoing[],
                      uint16_t n);

} // namespace worker
} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_WORKER_API_HPP_ */
