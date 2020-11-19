#include "config/op_config_file.hpp"
#include "framework/core/op_log.h"
#include "internal_server.hpp"
#include "firehose/server_tcp.hpp"
#include "firehose/server_udp.hpp"
#include "drivers/kernel.hpp"
//#include "drivers/dpdk.hpp"

namespace openperf::network::internal {

using protocol_t = model::protocol_t;

// Constructors & Destructor
server::server(const model::server& server_model)
    : model::server(server_model)
{
    OP_LOG(OP_LOG_INFO,
           "Starting %s server on port %ld\n",
           (protocol() == protocol_t::TCP) ? "TCP" : "UDP",
           port());

    static auto driver =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
            "modules.network.drivers");

    std::shared_ptr<drivers::network_driver> nd;
    if (!driver || !driver.value().compare(drivers::KERNEL)) {
        nd = std::make_shared<drivers::kernel>(drivers::kernel());
    } else {
        throw std::runtime_error("Network driver " + driver.value()
                                 + " is unsupported");
    }

    switch (protocol()) {
    case protocol_t::TCP:
        server_ptr = std::make_unique<firehose::server_tcp>(port(), nd);
        break;
    case protocol_t::UDP:
        server_ptr = std::make_unique<firehose::server_udp>(port(), nd);
        break;
    }

    server_ptr->run_accept_thread();
    server_ptr->run_worker_thread();
}

server::~server() { server_ptr.reset(nullptr); }

void server::reset() {}

} // namespace openperf::network::internal
