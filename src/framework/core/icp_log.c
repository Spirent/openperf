#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "zmq.h"

#include "core/icp_common.h"
#include "core/icp_list.h"
#include "core/icp_log.h"
#include "core/icp_socket.h"
#include "core/icp_thread.h"

static const char *icp_log_endpoint = "inproc://icp_logging";

static void *icp_log_context;
static enum icp_log_level icp_log_level;

static struct icp_list *_thread_log_sockets = NULL;
static __thread void *_log_socket = NULL;
static atomic_bool _log_thread_ready = ATOMIC_VAR_INIT(false);

#define THREAD_LENGTH ICP_THREAD_NAME_MAX_LENGTH

/**
 * Structure for sending messages to the logging thread
 */
struct icp_log_message {
    char thread[THREAD_LENGTH];  /**< Sender's thread name */
    time_t time;                 /**< Time sender logged message */
    const char *function;        /**< The name of the function that generated this message */
    char *message;               /**< A pointer to the message to log */
    enum icp_log_level level;    /**< The message's log level */
    uint32_t length;             /**< Length of the message (bytes) */
};

/**
 * Simple function to free strings that get logged via ZeroMQ;
 */
static void _zmq_buffer_free(void *data, void *hint __attribute__((unused)))
{
    free(data);
}

/**
 * Free dynamically allocated part of icp_log_message structs
 */
static void icp_log_free(const struct icp_log_message *msg)
{
    if (msg->function) free((void *)msg->function);
    if (msg->message) free((void *)msg->message);
}

/**
 * Get the system log level
 */
enum icp_log_level icp_log_level_get(void)
{
    return icp_log_level;
}

/**
 * Set the system log level
 */
void icp_log_level_set(enum icp_log_level level)
{
    if ((ICP_LOG_NONE <= level) && (level <= ICP_LOG_MAX))
        icp_log_level = level;
}

/**
 * Retrieve the logging socket for the calling thread.
 * If we don't have one, then create it and add it to our list of sockets.
 */
static
void * _get_thread_log_socket(void)
{
    if (_log_socket == NULL) {
        if ((_log_socket = zmq_socket(icp_log_context, ZMQ_PUSH)) == NULL
            || zmq_connect(_log_socket, icp_log_endpoint) != 0) {
            if (errno == ETERM) {
                return (NULL);  /* system is shutting down */
            }

            /* No good excuse for failing. */
            icp_exit("Could not connect to %s: %s\n", icp_log_endpoint,
                     zmq_strerror(errno));
        }

        /* Add this socket to our socket list, so we can close it on exit */
        if (!icp_list_insert(_thread_log_sockets, _log_socket)) {
            zmq_close(_log_socket);
            _log_socket = NULL;
        }
    }

    return (_log_socket);
}

static
void _close_zmq_socket(void *socket)
{
    zmq_close(socket);
}

/**
 * Take a message from an arbitrary context and send it to the logging
 * thread.  Not sure what to do about errors in this function, as the
 * inability to log messages probably ought to be fatal...
 */
int _icp_log(enum icp_log_level level, const char *function,
             const char *format, ...)
{
    va_list argp;
    va_start(argp, format);
    int error = icp_vlog(level, function, format, argp);
    va_end(argp);
    return (error);
}

int icp_vlog(enum icp_log_level level, const char *function,
             const char *format, va_list argp)
{
    int ret = 0;
    char *msg = NULL;
    void *messages = NULL;

    if (!atomic_load_explicit(&_log_thread_ready, memory_order_relaxed))
        return (0);

    if (level > icp_log_level)  /* Nothing to do */
        return (0);

    if ((messages = _get_thread_log_socket()) == NULL) {
        icp_safe_log("Logging message lost!  Could not retrieve thread log socket.\n");
        goto icp_log_exit;
    }

    /*
     * Make a copy of the string for our internal use
     * Note: The recv side will free the memory allocated by vasprintf
     */
    ret = vasprintf(&msg, format, argp);

    if (ret == -1) {
        /* Eek! We can't copy the log message for some reason */
        icp_safe_log("Logging message lost!  vasprintf call failed: %s", strerror(errno));
        goto icp_log_exit;
    }

    /* Populate the struct and forward to the logging thread */
    struct icp_log_message log = {
        .length = strlen(msg),
        .function = function ? strdup(function) : strdup("Unknown"),
        .message = msg,
        .level = level,
    };

    /* Get the thread's name */
    if (icp_thread_getname(pthread_self(), log.thread) != 0) {
        strncpy(log.thread, "unknown", THREAD_LENGTH);
    }

    /* Get the current time */
    time(&log.time);

    ret = zmq_send(messages, &log, sizeof(log), 0);
    if (ret != sizeof(log)) {
        icp_log_free(&log);
        if (errno != ETERM && errno != ENOTSOCK) {
            /* Filter out errors that occur when shutting down */
            icp_safe_log("Logging message lost!  zmq_send call failed: %s", zmq_strerror(errno));
        }
    }

    return (0);

icp_log_exit:
    return (ret);
}

void icp_log_write(const struct icp_log_message *msg, FILE *file, void *socket)
{
    char *level = NULL;

    switch(msg->level) {
    case ICP_LOG_DEBUG:
        level = "DEBUG";
        break;
    case ICP_LOG_INFO:
        level = "INFO";
        break;
    case ICP_LOG_WARNING:
        level = "WARNING";
        break;
    case ICP_LOG_ERROR:
        level = "ERROR";
        break;
    case ICP_LOG_CRITICAL:
        level = "CRITICAL";
        break;
    default:
        /* yikes! */
        level = "UNKNOWN";
        break;
    }

    /* Would we really be evil enough to send ourselves a NULL string? */
    if (msg->message) {
        int error = 0;
        char *log_msg = NULL;
        char txt_time[32];

        strftime(txt_time, sizeof(txt_time), "%FT%TZ", gmtime(&msg->time));
        error = asprintf(&log_msg, "%20s|%8s|%15s|%24.24s| %s",
                         txt_time, level, msg->thread,
                         msg->function, msg->message);

        if (error == -1) {
            /* Eek!  No message to log! */
            icp_safe_log("Could not create logging message with asprintf!\n");
            return;
        }

        /* Now log msg to file... */
        if (file) {
            fputs(log_msg, file);
        }

        /* ... and/or socket */
        if (socket) {
            zmq_msg_t zmq_msg;

            /*
             * ZeroMQ now owns the string buffer and will call
             * _zmq_buffer_free when the message has been sent.
             */
            if (zmq_msg_init_data(&zmq_msg, log_msg, strlen(log_msg) - 1,
                                  _zmq_buffer_free, NULL) != 0) {
                icp_safe_log("Could not initialize zmq_msg\n");
                free(log_msg);
                return;
            }

            zmq_msg_send(&zmq_msg, socket, ZMQ_DONTWAIT);
        } else {
            free(log_msg);
        }
    }
}

/**
 * Structure for sending initial arguments to the logging thread
 */
struct icp_log_task_args {
    void *context;
    const char *logging_endpoint;
    const char *pair_endpoint;
};

void *icp_log_task(void *void_args)
{
    struct icp_log_task_args *args = (struct icp_log_task_args *)void_args;
    int recv_or_err = 0;

    icp_thread_setname("icp_log");

    /*
     * Open socket back to our parent so we can send them a notification
     * that we're ready
     */
    void *parent = icp_socket_get_client(args->context, ZMQ_PAIR, args->pair_endpoint);
    assert(parent);

    /* Create a socket for pulling log messages from clients */
    void *messages = icp_socket_get_server(args->context, ZMQ_PULL, icp_log_endpoint);
    assert(messages);

    /* Create a socket for publishing log messages to external clients, if we have one */
    void *logs = NULL;
    if (args->logging_endpoint) {
        logs = icp_socket_get_server(args->context, ZMQ_PUB, args->logging_endpoint);
        assert(logs);
    }

    /*
     * Create a list for storing thread log sockets.  We need to close
     * them when we exit.
     */
    _thread_log_sockets = icp_list_allocate();
    assert(_thread_log_sockets);
    icp_list_set_destructor(_thread_log_sockets, _close_zmq_socket);

    /* Notify parent that we've started execution */
    zmq_send(parent, "", 0, 0);
    zmq_close(parent);

    /* We no longer need our arguments, so go ahead and free them */
    free(args);

    atomic_store(&_log_thread_ready, true);

    /* Enter our event loop; wait for messages and log any that come in */
    struct icp_log_message log;
    while ((recv_or_err = zmq_recv(messages, &log, sizeof(log), 0)) > 0) {
        if (recv_or_err == -1 && errno == ETERM) {
            break;
        } else if (recv_or_err == sizeof(log)) {
            /* Use stdout for now; can change later */
            icp_log_write(&log, stdout, logs);
            icp_log_free(&log);
        }
    }

    atomic_store(&_log_thread_ready, false);

    icp_list_free(&_thread_log_sockets);
    zmq_close(messages);
    if (logs) {
        zmq_close(logs);
    }

    return (NULL);
}

/**
 * Fire off the logging thread in a detached state
 */
int icp_log_init(void *context, const char *logging_endpoint)
{
    int ret = 0;
    pthread_t log_thread;
    pthread_attr_t log_thread_attr;
    const char *notify_endpoint = "inproc://icp_log_notify";
    struct icp_log_task_args *args = NULL;

    icp_log_context = context;

    /* Create a socket for the child ready notification */
    void *notify = zmq_socket(context, ZMQ_PAIR);
    assert(notify);
    if (zmq_bind(notify, notify_endpoint) != 0) {
        icp_exit("Failed to bind to log notification socket: %s\n",
                 zmq_strerror(errno));
    }

    /* Launch the logging thread */
    ret = pthread_attr_init(&log_thread_attr);
    assert(ret == 0);
    ret = pthread_attr_setdetachstate(&log_thread_attr, PTHREAD_CREATE_DETACHED);
    assert(ret == 0);

    args = (struct icp_log_task_args *)malloc(sizeof(*args));
    args->context = context;
    args->logging_endpoint = logging_endpoint;
    args->pair_endpoint = notify_endpoint;

    if ((ret = pthread_create(&log_thread, &log_thread_attr, &icp_log_task, args)) != 0) {
        free(args);
        return (-1);
    }

    /* Now wait for the child notification */
    zmq_pollitem_t item = {
        .socket = notify,
        .events = ZMQ_POLLIN,
    };

    while (zmq_poll(&item, 1, 1000) == 0) {
        icp_safe_log("Still waiting on logging thread to start...\n");
    }

    /* Clean up */
    zmq_close(notify);

    return (0);
}
