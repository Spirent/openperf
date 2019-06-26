#ifndef _ICP_PACKETIO_INTERNAL_API_H_
#define _ICP_PACKETIO_INTERNAL_API_H_

#include <string>

#include "packetio/generic_event_loop.h"

namespace icp::packetio::internal::api {

extern std::string_view endpoint;

struct callback_data {
        char name[64];
        event_loop::event_notifier notifier;
        event_loop::callback_function callback;
        std::any arg;
};

struct add_callback_msg {
    callback_data callback;
};

struct del_callback_msg {
    event_loop::event_notifier notifier;
};

}

#endif /* _ICP_PACKETIO_INTERNAL_API_H_ */
