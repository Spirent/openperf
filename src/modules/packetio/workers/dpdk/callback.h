#ifndef _ICP_PACKETIO_DPDK_CALLBACK_H_
#define _ICP_PACKETIO_DPDK_CALLBACK_H_

#include <string>

#include "packetio/generic_event_loop.h"
#include "packetio/workers/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class callback : public pollable_event<callback>
{
    event_loop::generic_event_loop m_loop;
    std::string m_name;
    event_loop::event_notifier m_notify;
    event_loop::callback_function m_callback;
    std::any m_arg;

public:
    callback(event_loop::generic_event_loop loop,
             std::string_view name,
             event_loop::event_notifier notifier,
             event_loop::callback_function callback,
             std::any arg);

    uint32_t poll_id() const;
    std::string_view name() const;
    event_loop::event_notifier notifier() const;

    void do_callback();

    int event_fd() const;
};

}

#endif /* _ICP_PACKETIO_DPDK_CALLBACK_H_ */
