#ifndef _ICP_PACKETIO_DPDK_WORKER_API_H_
#define _ICP_PACKETIO_DPDK_WORKER_API_H_

#include <memory>
#include <string>
#include <variant>

#include "lwip/netif.h"

#include "core/icp_core.h"
#include "net/net_types.h"
#include "packetio/vif_map.h"
#include "packetio/drivers/dpdk/queue_utils.h"

struct netif;

namespace icp {
namespace packetio {
namespace dpdk {
namespace worker {

struct start_msg {
    std::string endpoint;
};

struct stop_msg {
    std::string endpoint;
};

struct configure_msg {
    const std::vector<queue::descriptor> descriptors;
};

enum class message_type { NONE = 0,
                          CONFIG,
                          START,
                          STOP };

/**
 * Structure for sending commands to queue workers.  We use a simple POD
 * type to keep serialization/de-serialization simple.
 * I find it amazing that flexible arrays are not part of any c++ standard...
 * It literally has everything else you could possibly think of.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wflexible-array-extensions"
struct message {
    message_type type;
    union {
        struct {
            size_t endpoint_size;
            const char endpoint[];
        };
        struct {
            size_t descriptors_size;
            const queue::descriptor descriptors[];
        };
    };
};
#pragma clang diagnostic pop

extern const std::string endpoint;

class client
{
public:
    client(void *context, size_t nb_workers);

    void start();
    void stop();
    void configure(const std::vector<queue::descriptor>&);

private:
    const void*  m_context;
    const size_t m_workers;
    std::unique_ptr<void, icp_socket_deleter> m_socket;
};

struct args {
    void* context;
    std::string_view endpoint;
    std::shared_ptr<const vif_map<netif>> vif;
};

int main(void*);

}
}
}
}

#endif /* _ICP_PACKETIO_DPDK_WORKER_API_H_ */
