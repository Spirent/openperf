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

    tl::expected<std::string, int> add_task_impl(workers::context ctx,
                                                 std::string_view name,
                                                 event_loop::event_notifier notify,
                                                 event_loop::event_handler on_event,
                                                 std::optional<event_loop::delete_handler> on_delete,
                                                 std::any arg);
public:
    client(void* context);

    client(client&& other);
    client& operator=(client&& other);

    tl::expected<std::string, int> add_task(workers::context ctx,
                                            std::string_view name,
                                            event_loop::event_notifier notify,
                                            event_loop::event_handler on_event,
                                            std::any arg);

    tl::expected<std::string, int> add_task(workers::context ctx,
                                            std::string_view name,
                                            event_loop::event_notifier notify,
                                            event_loop::event_handler on_event,
                                            event_loop::delete_handler on_delete,
                                            std::any arg);

    tl::expected<void, int> del_task(std::string_view task_id);
};

}

#endif /* _ICP_PACKETIO_INTERNAL_CLIENT_H_ */
