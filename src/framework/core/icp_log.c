#include <assert.h>
#include <ctype.h>
#include <getopt.h>
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
#include "core/icp_event_loop.h"
#include "core/icp_list.h"
#include "core/icp_log.h"
#include "core/icp_options.h"
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
    const char* tag;             /**< Additional message information */
    const char* message;         /**< A pointer to the message to log */
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

static char* log_level_string(enum icp_log_level level)
{
    switch (level) {
    case ICP_LOG_CRITICAL:
        return "critical";
    case ICP_LOG_ERROR:
        return "error";
    case ICP_LOG_WARNING:
        return "warning";
    case ICP_LOG_INFO:
        return "info";
    case ICP_LOG_DEBUG:
        return "debug";
    case ICP_LOG_TRACE:
        return "trace";
    default:
        return "unknown";
    }
}

/**
 * Free dynamically allocated part of icp_log_message structs
 */
static void icp_log_free(const struct icp_log_message *msg)
{
    if (msg->tag) free((void *)msg->tag);
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

static enum icp_log_level parse_log_optarg(const char *arg)
{
    /* Check for a number */
    long level = strtol(arg, NULL, 10);
    if (level) {
        /* we found a number; clamp and return */
        if (level <= ICP_LOG_NONE) {
            return (ICP_LOG_CRITICAL);
        } else if (level >= ICP_LOG_MAX) {
            return (ICP_LOG_DEBUG);
        } else {
            return ((enum icp_log_level)level);
        }
        /* Guess it's garbage */
        return (ICP_LOG_NONE);
    }

    /* Normalize optarg */
    static size_t MAX_LEVEL_LENGTH = 8;
    char normal_arg[MAX_LEVEL_LENGTH];
    for (size_t i = 0; i < icp_min(strlen(arg), MAX_LEVEL_LENGTH); i++) {
        normal_arg[i] = isupper(arg[i]) ? tolower(arg[i]) : arg[i];
    }

    /* Look for known strings */
    for (enum icp_log_level ll = ICP_LOG_NONE; ll < ICP_LOG_MAX; ll++) {
        const char *ref = log_level_string(ll);
        if (strncmp(normal_arg, ref, icp_min(strlen(ref), MAX_LEVEL_LENGTH)) == 0) {
            return (ll);
        }
    }

    return (ICP_LOG_NONE);
}

enum icp_log_level icp_log_level_find(int argc, char * const argv[])
{
    for (int idx = 0; idx < argc - 1; idx++) {
        if (strcmp(argv[idx], "--log-level") == 0
            || strcmp(argv[idx], "-l") == 0) {
            return parse_log_optarg(argv[idx + 1]);
        }
    }

    return (ICP_LOG_NONE);
}

static const char * skip_keywords(const char *begin, const char *end)
{
    /* Various words that precede or modify the type that should also be skipped */
    static const char* keywords[] = {
        "unsigned",
        "signed",
        "static"
    };

    const char *cursor = begin;
    size_t length = end - begin;
    for (size_t i = 0; i < icp_count_of(keywords); i++) {
        const char *keyword = keywords[i];
        if (length > strlen(keyword) && strncmp(cursor, keyword, strlen(keyword)) == 0) {
            size_t adjust = strlen(keyword) + 1;
            cursor += adjust;
            assert(length >= adjust);
            length -= adjust;
        }
    }

    /*
     * We might have some c++ code, so handle templates and ctors/dtors, which
     * don't have return types.
     */
    size_t template_level = 0;
    const char *to_return = cursor;
    while (cursor <= end) {
        if (*cursor == '(' && template_level == 0) {
            break;
        }
        if (*cursor == ' ' && template_level == 0) {
            to_return = cursor;
            break;
        }
        if (*cursor == '<') {
            template_level++;
        }
        if (*cursor == '>' && template_level > 0) {
            template_level--;
        }
        cursor++;
    }

    /* Skip any trailing spaces so we can return a cursor pointing at something */
    while (*to_return == ' ') to_return++;

    return (to_return);
}

void icp_log_function_name(const char *signature, char *function)
{
    char const *cursor = signature;
    char const *end = signature + strlen(signature);

    /* Skip over the return type */
    cursor = skip_keywords(cursor, end);
    if (cursor == end) {
        cursor = signature;
        goto function_name_exit;
    }

    if (*cursor == '(') {
        /* Found a function; skip the function's return type */
        cursor = skip_keywords(++cursor, end);

        if (cursor == end) {
            cursor = signature;
            goto function_name_exit;
        }
    }

    /*
     * Should be at the beginning of the actual function name.  Look for
     * the first opening '(' which marks the end of the function name.
     */
    end = strchr(cursor, '(');

function_name_exit:
    memcpy(function, cursor, end - cursor);
    function[end - cursor] = 0; /* null terminate string */
}

/**
 * Retrieve the logging socket for the calling thread.
 * If we don't have one, then create it and add it to our list of sockets.
 */
static void * get_thread_log_socket(void)
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

/**
 * Take a message from an arbitrary context and send it to the logging
 * thread.  Not sure what to do about errors in this function, as the
 * inability to log messages probably ought to be fatal...
 */
int icp_log(enum icp_log_level level, const char *tag,
            const char *format, ...)
{
    va_list argp;
    va_start(argp, format);
    int error = icp_vlog(level, tag, format, argp);
    va_end(argp);
    return (error);
}

int icp_vlog(enum icp_log_level level, const char *tag,
             const char *format, va_list argp)
{
    int ret = 0;
    char *msg = NULL;
    void *messages = NULL;

    if (!atomic_load_explicit(&_log_thread_ready, memory_order_relaxed))
        return (0);

    if (level > icp_log_level)  /* Nothing to do */
        return (0);

    if ((messages = get_thread_log_socket()) == NULL) {
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
        .tag = tag ? strdup(tag) : strdup("none"),
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
    if (!msg->message || msg->length == 0) {
        return;
    }

    int error = 0;
    char *log_msg = NULL;
    char txt_time[32];

    strftime(txt_time, sizeof(txt_time), "%FT%TZ", gmtime(&msg->time));
    /* Note: we use the length to trim the trailing '\n' from the message */
    error = asprintf(&log_msg,
                     "{\"time\": \"%s\", \"level\": \"%s\", \"thread\": \"%s\", "
                     "\"tag\": \"%s\", \"message\": \"%.*s\"}\n",
                     txt_time, log_level_string(msg->level), msg->thread, msg->tag,
                     msg->message[msg->length - 1] == '\n' ? msg->length - 1 : msg->length,
                     msg->message);
    if (error == -1) {
        /* Eek!  No message to log! */
        icp_safe_log("Could not create logging message with asprintf!\n");
        return;
    }

    /* Now log msg to file... */
    if (file) {
        fputs(log_msg, file);
        fflush(file);
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

/**
 * Default logging callback handler
 */
static int handle_message(const struct icp_event_data *data, void *socket)
{
    struct icp_log_message log;
    int recv_or_err = 0;
    while ((recv_or_err = zmq_recv(data->socket, &log, sizeof(log), ZMQ_DONTWAIT))
           == sizeof(log)) {
        /* Use stdout for now; can change later */
        icp_log_write(&log, stdout, socket);
        icp_log_free(&log);
    }

    return ((recv_or_err < 0 && errno == ETERM) ? -1 : 0);
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
    struct icp_log_task_args *args = (struct icp_log_task_args*)void_args;

    icp_thread_setname("icp_log");

    /*
     * Open socket back to our parent so we can send them a notification
     * that we're ready.
     */
    void *parent = icp_socket_get_client(args->context, ZMQ_PAIR, args->pair_endpoint);
    if (!parent) icp_exit("Could not create parent notification socket\n");

    /* Create a socket for pulling log messages from clients */
    void *messages = icp_socket_get_server(args->context, ZMQ_PULL, icp_log_endpoint);
    if (!messages) icp_exit("Could not create logging server socket\n");

    /* Create a socket for publishing log messages to external clients, maybe */
    void *external = NULL;
    if (args->logging_endpoint) {
        external = icp_socket_get_server(args->context, ZMQ_PUB, args->logging_endpoint);
        if (!external) icp_exit("Could not create log publishing socket\n");
    }

    /* Create an event loop to control our actions */
    struct icp_event_loop* loop = icp_event_loop_allocate();
    if (!loop) icp_exit("Could not create event loop\n");

    struct icp_event_callbacks callbacks = {
        .on_read = handle_message,
    };

    int error = icp_event_loop_add(loop, messages, &callbacks, external);
    if (error) icp_exit("Could not add callack to event loop\n");

    /*
     * Create a list for storing thread local log sockets.  We will need to close
     * them all when we exit.
     */
    _thread_log_sockets = icp_list_allocate();
    if (!_thread_log_sockets) icp_exit("Could not create thread local socket storage list\n");
    icp_list_set_destructor(_thread_log_sockets, icp_socket_close);

    /* Notify parent that we're ready to work */
    zmq_send(parent, "", 0, 0);
    zmq_close(parent);

    /* We no longer need our arguments, so go ahead and free them */
    free(args);

    atomic_store(&_log_thread_ready, true);

    /* Start logging! */
    icp_event_loop_run(loop);

    atomic_store(&_log_thread_ready, false);

    icp_list_free(&_thread_log_sockets);
    zmq_close(messages);
    if (external) zmq_close(external);

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
    if (!notify) icp_exit("Failed to create notification socket\n");

    if (zmq_bind(notify, notify_endpoint) != 0) {
        icp_exit("Failed to bind to log notification socket: %s\n",
                 zmq_strerror(errno));
    }

    /* Launch the logging thread */
    if (pthread_attr_init(&log_thread_attr)
        || pthread_attr_setdetachstate(&log_thread_attr, PTHREAD_CREATE_DETACHED)) {
        icp_exit("Could not set pthread attributes\n");
    }

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

/*
 * Register the log-level option.
 * Note: we don't register any callbacks because we parse them before the
 * normal call to icp_potion_parse.  This is so we can set the proper log
 * level before doing any real work.
 */
static struct icp_options_data log_level_option = {
    .name = "log-level",
    .init = NULL,
    .callback = NULL,
    .options = {
        { "Specify the log level; takes a number (1-6) or level", "log-level", 'l', true , ICP_OPTION_TYPE_LONG},
        { 0, 0, 0, 0, 0 },
    }
};

REGISTER_OPTIONS(log_level_option)
