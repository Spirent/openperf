#ifndef _ICP_PACKETIO_INTERNAL_SERVER_H_
#define _ICP_PACKETIO_INTERNAL_SERVER_H_

#include <memory>

#include "core/icp_core.h"
#include "packetio/generic_workers.h"

namespace icp::packetio::internal::api {

class server
{
    std::unique_ptr<void, icp_socket_deleter> m_socket;
    workers::generic_workers& m_workers;

public:
    server(void* context,
           core::event_loop& loop,
           workers::generic_workers& workers);

    workers::generic_workers& workers() const;
};

}

#endif /* _ICP_PACKETIO_INTERNAL_SERVER_H_ */
