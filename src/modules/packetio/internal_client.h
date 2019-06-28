#ifndef _ICP_PACKETIO_INTERNAL_CLIENT_H_
#define _ICP_PACKETIO_INTERNAL_CLIENT_H_

#include <any>
#include <memory>
#include "tl/expected.hpp"

#include "core/icp_core.h"
#include "packetio/generic_event_loop.h"
#include "packetio/generic_workers.h"

namespace icp::packetio::internal::api {

class client
{
    std::unique_ptr<void, icp_socket_deleter> m_socket;

public:
    client(void* context);

    tl::expected<std::string, int> add_task(std::string_view name,
                                            workers::context ctx,
                                            event_loop::event_notifier notify,
                                            event_loop::callback_function callback,
                                            std::any arg);

    tl::expected<void, int> del_task(std::string_view task_id);
};

}

#endif /* _ICP_PACKETIO_INTERNAL_CLIENT_H_ */
