#include "framework/core/op_log.h"
#include "internal_server.hpp"
#include "firehose/server_tcp.hpp"

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

    switch (protocol()) {
    case protocol_t::TCP:
        server_ptr = std::make_unique<firehose::server_tcp>(port());
        break;
    case protocol_t::UDP:
        /* code */
        break;
    }

    server_ptr->run_accept_thread();
    server_ptr->run_worker_thread();
}

server::~server() { server_ptr.reset(nullptr); }

void server::reset() {}

} // namespace openperf::network::internal
