#ifndef _OP_PACKETIO_INTERNAL_SERVER_HPP_
#define _OP_PACKETIO_INTERNAL_SERVER_HPP_

#include <memory>

#include "core/op_core.h"
#include "packetio/generic_workers.hpp"

namespace openperf::packetio::internal::api {

class server
{
    std::unique_ptr<void, op_socket_deleter> m_socket;
    workers::generic_workers& m_workers;

public:
    server(void* context,
           core::event_loop& loop,
           workers::generic_workers& workers);

    workers::generic_workers& workers() const;
};

} // namespace openperf::packetio::internal::api

#endif /* _OP_PACKETIO_INTERNAL_SERVER_HPP_ */
