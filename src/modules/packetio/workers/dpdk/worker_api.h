#ifndef _ICP_PACKETIO_DPDK_WORKER_API_H_
#define _ICP_PACKETIO_DPDK_WORKER_API_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "lwip/netif.h"
#include "packetio/vif_map.h"

namespace icp::packetio::dpdk {
class callback;
class rx_queue;
class tx_queue;
class zmq_socket;

namespace worker {

using fib = vif_map<netif>;

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

struct configure_msg {
    std::vector<worker::descriptor> descriptors;
};

/**
 * Our worker is a finite state machine and these messages are
 * the events that trigger state changes.
 */
using command_msg = std::variant<start_msg, stop_msg, configure_msg>;

extern const std::string_view endpoint;

class client
{
public:
    client(void *context, size_t nb_workers);

    void start();
    void stop();
    void configure(const std::vector<worker::descriptor>&);

private:
    const void*  m_context;
    const size_t m_workers;
    std::unique_ptr<void, icp_socket_deleter> m_socket;
};

struct main_args {
    void* context;
    std::string_view endpoint;
    const fib* fib;
};

int main(void*);

int send_message(void* socket, const command_msg& msg);
std::optional<command_msg> recv_message(void* socket, int flags = 0);

}
}

#endif /* _ICP_PACKETIO_DPDK_WORKER_API_H_ */
