#include <cassert>
#include <string>
#include <thread>
#include <unordered_map>

#include <zmq.h>

#include "core/icp_config_server.h"
#include "core/icp_event_loop.hpp"
#include "core/icp_log.h"
#include "core/icp_modules.h"
#include "core/icp_socket.h"
#include "core/icp_thread.h"

namespace icp {
namespace core {
namespace config {

static __attribute__((const)) const char *
_request_type_string(enum icp_config_server_request_type type)
{
    switch (type) {
#define CASE_MAP(t, s) case t: return s
        CASE_MAP(REQUEST_PING, "PING");
        CASE_MAP(REQUEST_REG,  "REGISTER");
        CASE_MAP(REQUEST_ADD,  "ADD");
        CASE_MAP(REQUEST_DEL,  "DELETE");
        CASE_MAP(REQUEST_GET,  "GET");
        CASE_MAP(REQUEST_SET,  "SET");
#undef CASE_MAP
    }
    return "UNKNOWN";
}

static __attribute__((const)) const char *
_reply_code_string(enum icp_config_server_response_code code)
{
    switch (code) {
#define CASE_MAP(t, s) case t: return s
        CASE_MAP(RESPONSE_OK, "OK");
        CASE_MAP(RESPONSE_ERROR, "ERROR");
        CASE_MAP(RESPONSE_TRY_AGAIN, "TRY_AGAIN");
        CASE_MAP(RESPONSE_UNKNOWN_KEY, "UNKNOWN_KEY");
#undef CASE_MAP
    }
    return "UNKNOWN";
}

static int _handle_rpc_request(const icp_event_data *data,
                               void *arg);

struct service {
    ~service() {
        if (worker.joinable()) {
            worker.join();
        }
    }

    int init(void *context, std::string endpoint) {
        socket.reset(icp_socket_get_server(context, ZMQ_REP, endpoint.c_str()));
        assert(socket);

        loop.reset(new icp::core::event_loop());
        assert(loop);

        map.reset(new zmq_socket_map(context));
        assert(map);

        struct icp_event_callbacks callbacks = {
            .on_read = _handle_rpc_request
        };

        if (int error = loop->add(socket.get(), &callbacks, this)) {
            return (error);
        }

        /* Run the loop in a new thread and return control back to caller */
        worker = std::thread([this]()
                             {
                                 icp_thread_setname("icp_config");
                                 loop->run();
                             });
        return (0);
    }

    /**
     * Utility structs and typedefs
     */
    struct zmq_socket_closer {
        void operator()(void *s) const {
            zmq_close(s);
        }
    };

    typedef std::unique_ptr<void, zmq_socket_closer> socket_ptr;

    struct zmq_socket_map {
        zmq_socket_map(void *_context)
            : context(_context) {}

        void * get(std::string endpoint)
        {
            if (map.find(endpoint) == map.end()) {
                map[endpoint] = socket_ptr(icp_socket_get_client(context, ZMQ_REQ, endpoint.c_str()));
            }
            return map[endpoint].get();
        }

        void *context;
        std::unordered_map<std::string, socket_ptr> map;
    };

    socket_ptr socket;
    std::unique_ptr<icp::core::event_loop> loop;
    std::thread worker;
    std::unique_ptr<zmq_socket_map> map;
};

static void _handle_ping_request(service *s __attribute__((unused)),
                                 const struct icp_config_server_request &request __attribute__((unused)),
                                 struct icp_config_server_reply &reply)
{
    reply.code = RESPONSE_OK;
}

static int _handle_register_request(service *s,
                                    const struct icp_config_server_request &request,
                                    struct icp_config_server_reply &reply)
{

}

static int _handle_rpc_request(const icp_event_data *data,
                               void *arg)
{
    struct service *s = reinterpret_cast<struct service *>(arg);
    (void)s;

    struct icp_config_server_request request = {};
    int recv_or_err = 0;
    int send_or_err = 0;

    while ((recv_or_err = zmq_recv(data->socket, &request, sizeof(request), ZMQ_DONTWAIT))
           == sizeof(request)) {
        icp_log(ICP_LOG_TRACE, "Received %s request for %s\n",
                _request_type_string(request.type), request.key);

        struct icp_config_server_reply reply = {};

        switch (request.type) {
        case REQUEST_PING:
            _handle_ping_request(s, request, reply);
            break;
        case REQUEST_REG:
            _handle_register_request(s, request, reply);
            break;
        case REQUEST_ADD:
            break;
        case REQUEST_DEL:
            break;
        case REQUEST_GET:
            break;
        case REQUEST_SET:
            break;
        default:
            reply.code = RESPONSE_ERROR;
        }

        icp_log(ICP_LOG_TRACE, "Sending %s response for %s request for %s\n",
                _reply_code_string(reply.code),
                _request_type_string(request.type), request.key);

        if ((send_or_err = zmq_send(data->socket, &reply, sizeof(reply), 0))
             != sizeof(reply)) {
            icp_log(ICP_LOG_ERROR, "Request reply failed: %s",
                    zmq_strerror(errno));
            break;
        }
    }

    return (((recv_or_err < 0 || send_or_err < 0) && errno == ETERM) ? -1 : 0);
}


}
}
}

extern "C" {

int icp_config_service_init(void *context, void *state)
{
    icp::core::config::service *s = reinterpret_cast<icp::core::config::service *>(state);
    return (s->init(context, "inproc://icp_configure"));
}

struct icp_module config_service = {
    .name = "config service",
    .state = new icp::core::config::service(),
    .init = icp_config_service_init,
    .start = nullptr,
};

REGISTER_MODULE(config_service)

}
