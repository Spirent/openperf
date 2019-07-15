#ifndef _ICP_PACKETIO_DPDK_CALLBACK_H_
#define _ICP_PACKETIO_DPDK_CALLBACK_H_

#include <string>

#include "packetio/generic_event_loop.h"
#include "packetio/workers/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class callback : public pollable_event<callback>
{
    std::string m_name;
    event_loop::event_notifier m_notify;
    event_loop::event_handler m_on_event;
    std::optional<event_loop::delete_handler> m_on_delete;
    std::any m_arg;

public:
    callback(std::string_view name,
             event_loop::event_notifier notifier,
             event_loop::event_handler on_event,
             std::optional<event_loop::delete_handler> on_delete,
             std::any arg);
    ~callback();

    std::string_view name() const;
    event_loop::event_notifier notifier() const;
    int event_fd() const;

    void run_callback(event_loop::generic_event_loop& loop);
};

}

#endif /* _ICP_PACKETIO_DPDK_CALLBACK_H_ */
