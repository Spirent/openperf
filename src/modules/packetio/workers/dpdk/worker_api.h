#ifndef _ICP_PACKETIO_DPDK_WORKER_API_H_
#define _ICP_PACKETIO_DPDK_WORKER_API_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "core/icp_core.h"
#include "lwip/netif.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/forwarding_table.h"
#include "packetio/generic_sink.h"
#include "packetio/recycle.h"
#include "packetio/transmit_table.h"
#include "packetio/workers/dpdk/tx_source.h"

namespace icp::packetio::dpdk {
class callback;
class rx_queue;
class tx_queue;
class tx_scheduler;
class zmq_socket;

namespace worker {

static constexpr uint16_t pkt_burst_size = 32;

using fib = packetio::forwarding_table<netif,
                                       packets::generic_sink,
                                       RTE_MAX_ETHPORTS>;
using tib = packetio::transmit_table<dpdk::tx_source>;
using recycler = packetio::recycle::depot<RTE_MAX_LCORE>;

/**
 * The types of things workers know how to deal with.
 */
using task_ptr = std::variant<callback*,
                              rx_queue*,
                              tx_queue*,
                              tx_scheduler*,
                              zmq_socket*>;

/**
 * The types of things we insert into descriptors.
 */
using descriptor_ptr = std::variant<callback*,
                                    rx_queue*,
                                    tx_queue*,
                                    tx_scheduler*>;

struct descriptor {
    uint16_t worker_id;
    descriptor_ptr ptr;

    descriptor(uint16_t id, descriptor_ptr p)
        : worker_id(id)
        , ptr(p)
    {}
};

struct start_msg {
    std::string endpoint;
};

struct stop_msg {
    std::string endpoint;
};

struct add_descriptors_msg {
    std::vector<worker::descriptor> descriptors;
};

struct del_descriptors_msg {
    std::vector<worker::descriptor> descriptors;
};

/**
 * Our worker is a finite state machine and these messages are
 * the events that trigger state changes.
 */
using command_msg = std::variant<start_msg,
                                 stop_msg,
                                 add_descriptors_msg,
                                 del_descriptors_msg>;

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
    std::unique_ptr<void, icp_socket_deleter> m_socket;
};

struct main_args {
    void* context;
    std::string_view endpoint;
    recycler* recycler;
    const fib* fib;
};

int main(void*);

int send_message(void* socket, const command_msg& msg);
std::optional<command_msg> recv_message(void* socket, int flags = 0);

}
}

#endif /* _ICP_PACKETIO_DPDK_WORKER_API_H_ */
