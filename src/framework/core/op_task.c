#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "zmq.h"

#include "core/op_common.h"
#include "core/op_log.h"
#include "core/op_socket.h"
#include "core/op_task.h"
#include "core/op_thread.h"

struct op_task_internal_args {
    task_fn *original_task;
    struct op_task_args *original_args;
    char *pair_endpoint;
};


/**
 * A generic task wrapper that notifies the caller that is has
 * been started
 */
static void *op_task_launcher_task(void *void_args)
{
    struct op_task_internal_args *iargs =
        (struct op_task_internal_args *)void_args;
    task_fn *task = iargs->original_task;
    struct op_task_args *args = iargs->original_args;

    /* Set thread name */
    op_thread_setname(args->name);

    /* Notify parent that we've started execution */
    if (op_task_sync_ping(args->context, iargs->pair_endpoint) != 0) {
        op_exit("Failed to notify parent that %s thread has started\n",
                 args->name);
    }

    /* Clean up */
    free(iargs->pair_endpoint);
    free(iargs);

    /* Now jump into original task */
    return task(args);
}

/**
 * Start a generic task
 */
int op_task_launch(task_fn *task,
                    struct op_task_args *task_args,
                    const char *task_name)
{
    pthread_t thread;
    pthread_attr_t thread_attr;
    int err = 0;

    struct op_task_internal_args *iargs = malloc(sizeof(*iargs));
    assert(iargs);

    /* Create a socket for the child notification */
    asprintf(&iargs->pair_endpoint, "inproc://op_%s_notify", task_name);
    assert(iargs->pair_endpoint);
    void *notify = op_task_sync_socket(task_args->context,
                                        iargs->pair_endpoint);
    assert(notify);

    /* Launch a detached thread */
    int error = pthread_attr_init(&thread_attr);
    assert(!error);
    error = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    assert(!error);

    /* Populate internal argument structure */
    iargs->original_task = task;
    iargs->original_args = task_args;

    OP_LOG(OP_LOG_DEBUG, "Creating new thread for %s task...\n", task_name);

    if ((err = pthread_create(&thread, &thread_attr, op_task_launcher_task, iargs)) != 0) {
        OP_LOG(OP_LOG_ERROR, "Failed to launch task %s: %d\n",
                task_name, err);
        goto error_and_free;
    }

    op_task_sync_block_and_warn(&notify, 1, 1000,
                                 "Still waiting on acknowledgment from %s task\n",
                                 task_name);
    OP_LOG(OP_LOG_DEBUG, "Task %s started!\n", task_name);

    return (0);

error_and_free:
    zmq_close(notify);

    /*
     * An error means the child didn't do anything; so clean up the things
     * it should have cleaned up.
     */
    free(iargs->pair_endpoint);
    free(iargs);
    free(task_args);

    return (err);
}

void *op_task_sync_socket(void *context, const char *endpoint)
{
    return (op_socket_get_server(context, ZMQ_PULL, endpoint));
}

static void _op_task_unbind_socket(void *socket)
{
    /*
     * Explicitly unbind the socket so we can reuse the endpoint immediately
     * if necessary.  The loop is so that we can be sure to create a large
     * enough buffer for the endpoint.  There doesn't appear to be a way
     * to query the size directly.
     */
    size_t length = 64;
    for (;;) {
        char buffer[length];
        if (zmq_getsockopt(socket, ZMQ_LAST_ENDPOINT, buffer, &length) == 0) {
            zmq_unbind(socket, buffer);
            break;
        } else if (errno == EINVAL) {
            length <<= 1;
        } else {
            break;
        }
    }
}

int op_task_sync_block(void **socketp, size_t expected)
{
    void *socket = *socketp;
    size_t responses = 0;
    uint8_t buffer[16];
    while (responses < expected
           && zmq_recv(socket, buffer, sizeof(buffer), 0) >= 0
           && errno != ETERM) {
        responses++;
    }

    _op_task_unbind_socket(socket);
    zmq_close(socket);
    *socketp = NULL;

    return (0);
}

int _op_task_sync_block_and_warn(void **socketp, size_t expected,
                                  unsigned interval,
                                  const char *function, const char *format, ...)
{
    void *socket = *socketp;
    unsigned responses = 0;
    int error = 0;
    while (responses < expected) {
        unsigned waited = 0;
        while (waited < interval) {
            /*
             * XXX: Our clock service might not be started, so use
             * the host clock to be safe.
             */
            struct timeval start = {}, stop = {}, delta = {};
            gettimeofday(&start, NULL);

            zmq_pollitem_t item = {
                .socket = socket,
                .events = ZMQ_POLLIN | ZMQ_POLLERR,
            };

            if (zmq_poll(&item, 1, interval - waited) == -1) {
                errno = -errno;
                goto _op_task_sync_block_and_warn_out;
            } else {
                if (zmq_recv(socket, NULL, 0, 0) == -1) {
                    error = -errno;
                    goto _op_task_sync_block_and_warn_out;
                }
                /* assume we got a response */
                if (++responses == expected) break;
            }

            gettimeofday(&stop, NULL);
            timersub(&stop, &start, &delta);

            waited += (delta.tv_sec * 1000) + (delta.tv_usec / 1000);
        }

        if (responses < expected) {
            va_list argp;
            va_start(argp, format);
            op_vlog(OP_LOG_WARNING, function, format, argp);
            va_end(argp);
        }
    }

    assert(responses == expected);

_op_task_sync_block_and_warn_out:
    _op_task_unbind_socket(socket);
    zmq_close(socket);
    *socketp = NULL;
    return (error);
}

int op_task_sync_ping(void *context, const char *endpoint)
{
    void *sync = op_socket_get_client(context, ZMQ_PUSH,
                                       endpoint);
    if (!sync) {
        return (-1);
    }

    int send_or_err = zmq_send(sync, "", 0, 0);
    zmq_close(sync);

    return (send_or_err >= 0 ? 0 : -1);
}
