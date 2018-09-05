#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "core/icp_common.h"
#include "core/icp_event_loop.h"
#include "core/icp_log.h"
#include "core/icp_socket.h"
#include "core/icp_task.h"

#include "civetweb.h"
#include "zmq.h"

const char *icp_config_endpoint = "inproc://icp_configure";

struct rest_server_state {
    struct icp_event_loop *loop;
    struct mg_context *mg_ctx;
    void * zmq_ctx;
    void * service;
};

/*
 * Civetweb callbacks
 */
static int _rest_begin_request(struct mg_connection *conn)
{
    icp_log(ICP_LOG_DEBUG, "HTTP begin request (%p)\n", (void *)conn);
    return (0);
}

static void _rest_end_request(const struct mg_connection *conn, int reply_status_code)
{
    icp_log(ICP_LOG_DEBUG, "HTTP end request %p:%d\n", (const void *)conn, reply_status_code);
}

static int _rest_http_error(struct mg_connection *conn, int status)
{
    icp_log(ICP_LOG_ERROR, "HTTP error %p:%d\n", (void *)conn, status);
    return (0);
}

static int _rest_log_message(const struct mg_connection *conn, const char *message)
{
    icp_log(ICP_LOG_INFO, "%p: %s\n", (const void *)conn, message);
    return (1);
}

/*
 * ZeroMQ service socket callbacks
 */
static int _handle_rpc_request(const struct icp_event_data *data,
                               void *private)
{
    struct rest_server_state *state = private;
    (void)state;

    int recv_or_err = 0;
    while ((recv_or_err = zmq_recv(data->socket, NULL, 0, ZMQ_DONTWAIT)) > 0) {
        icp_log(ICP_LOG_INFO, "Request received\n");
    }

    return ((recv_or_err < 0 && errno == ETERM) ? -1 : 0);
}

/*
 * Service task
 */
void * icp_rest_server_task(void *void_args)
{
    struct rest_server_state state = {};

    struct icp_task_args *args = (struct icp_task_args *)void_args;
    state.zmq_ctx = args->context;
    free(args);

    /* Initialize the event loop */
    if ((state.loop = icp_event_loop_allocate()) == NULL) {
        icp_exit("Could not allocate REST server event loop");
    }

    unsigned mg_features = 8 | 16;
    if (mg_init_library(mg_features) == 0) {
        icp_exit("Could not initialize REST web server");
    }

    /* TODO: civetweb options/calbacks */
    const char *options[] = {
        "listening_ports", "8080",
        "num_threads", "4",
        NULL
    };
    struct mg_callbacks callbacks = {
        .begin_request = _rest_begin_request,
        .end_request   = _rest_end_request,
        .http_error    = _rest_http_error,
        .log_message   = _rest_log_message,
    };
    if ((state.mg_ctx = mg_start(&callbacks, 0, options)) == NULL) {
        icp_exit("Could not start REST server");
    }

    /* Create a ZeroMQ service socket */
    if ((state.service = icp_socket_get_server(state.zmq_ctx, ZMQ_REP,
                                               icp_config_endpoint)) == NULL) {
        icp_exit("Could not create internal REST config socket");
    }

    const struct icp_event_callbacks rpc_callbacks = {
        .on_read = _handle_rpc_request,
    };
    icp_event_loop_add(state.loop, state.service, &rpc_callbacks, &state);

    /* Enter event loop */
    icp_event_loop_run(state.loop);

    /* Clean up */
    mg_stop(state.mg_ctx);
    mg_exit_library();
    zmq_close(state.service);

    return (NULL);
}

int icp_rest_server_init(void *context)
{
    struct icp_task_args *targs = malloc(sizeof(*targs));
    if (!targs) {
        return (-ENOMEM);
    }

    targs->context = context;
    strcpy(targs->name, "icp_config");

    int err = icp_task_launch(icp_rest_server_task, targs,
                              "REST configuration service");
    if (err) {
        free(targs);
        return (err);
    }

    return (0);
}
