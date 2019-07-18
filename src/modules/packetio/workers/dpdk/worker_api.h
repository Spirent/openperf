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
#include "packetio/generic_source.h"
#include "packetio/recycle.h"

namespace icp::packetio::dpdk {
class callback;
class rx_queue;
class tx_queue;
class zmq_socket;

namespace worker {

using fib = packetio::forwarding_table<netif,
                                       packets::generic_sink,
                                       RTE_MAX_ETHPORTS>;
using recycler = packetio::recycle::depot<RTE_MAX_LCORE>;

/**
 * The types of things the worker knows how to handle.
 */
using task_ptr = std::variant<callback*,
                              rx_queue*,
                              tx_queue*,
                              zmq_socket*>;

struct descriptor {
    uint16_t worker_id;
    task_ptr task;

    descriptor(uint16_t id, task_ptr ptr)
        : worker_id(id)
        , task(ptr)
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
