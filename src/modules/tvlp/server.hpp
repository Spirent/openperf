#ifndef _OP_TVLP_SERVER_H_
#define _OP_TVLP_SERVER_H_

#include "api.hpp"

#include "framework/core/op_core.h"

namespace openperf::tvlp::api {

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    server(void* context, core::event_loop& loop);

    int handle_rpc_request(const op_event_data* data);

private:
    api_reply handle_request(const message&);
};

} // namespace openperf::tvlp::api

#endif // _OP_MEMORY_SERVER_H_
