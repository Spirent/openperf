#include "config/op_config_file.hpp"
#include "framework/core/op_log.h"
#include "internal_server.hpp"

#include "firehose/server_tcp.hpp"
#include "firehose/server_udp.hpp"

#include "drivers/kernel.hpp"
#include "drivers/dpdk.hpp"

namespace openperf::network::internal {

using protocol_t = model::protocol_t;

// Constructors & Destructor
server::server(const model::server& server_model)
    : model::server(server_model)
{
    OP_LOG(OP_LOG_DEBUG,
           "Starting %s server on port %hu\n",
           (protocol() == protocol_t::TCP) ? "TCP" : "UDP",
           port());

    static auto driver =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
            "modules.network.driver");

    std::shared_ptr<drivers::driver> nd;
    if (!driver || !driver.value().compare(drivers::kernel::key)) {
        nd = drivers::driver::instance<drivers::kernel>();
    } else if (!driver.value().compare(drivers::dpdk::key)) {
        nd = drivers::driver::instance<drivers::dpdk>();
    } else {
        throw std::runtime_error("Network driver " + driver.value()
                                 + " is unsupported");
    }

    if (!m_interface && nd->bind_to_device_required())
        throw std::runtime_error(
            "Bind to the interface is required for this driver");

    switch (protocol()) {
    case protocol_t::TCP:
        server_ptr =
            std::make_unique<firehose::server_tcp>(port(), m_interface, nd);
        break;
    case protocol_t::UDP:
        server_ptr =
            std::make_unique<firehose::server_udp>(port(), m_interface, nd);
        break;
    }
}

server::~server() { server_ptr.reset(nullptr); }

void server::reset() {}

server::stat_t server::stat() const
{
    auto& istat = server_ptr->stat();
    auto& stat = const_cast<stat_t&>(m_stat);
    stat.connections = istat.connections;
    stat.errors = istat.errors;
    stat.bytes_received = istat.bytes_received;
    stat.closed = istat.closed;
    stat.bytes_sent = istat.bytes_sent;
    return m_stat;
}

} // namespace openperf::network::internal
