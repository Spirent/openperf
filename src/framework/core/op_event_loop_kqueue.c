#include <assert.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <sys/event.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/types.h>

#include <zmq.h>

#include "core/op_common.h"
#include "core/op_event_loop_utils.h"
#include "core/op_log.h"

static const size_t KQUEUE_MAX_EVENTS = 1024;

enum op_event_type {
    OP_EVENT_TYPE_NONE = 0,
    OP_EVENT_TYPE_FD,
    OP_EVENT_TYPE_ZMQ,
    OP_EVENT_TYPE_TIMER,
};

#define OP_EVENT_DELETED (1 << 0)

struct op_event
{
    struct op_event_data data;
    struct op_event_callbacks callbacks;
    void* arg;
    int flags;
    SLIST_ENTRY(op_event) events_link;
    SLIST_ENTRY(op_event) update_link;
    SLIST_ENTRY(op_event) remove_link;
    SLIST_ENTRY(op_event) disable_link;
};

#define OP_EVENT_LOOP_RUNNING (1 << 0)
#define OP_EVENT_LOOP_EDGETRIGGER (1 << 1)

struct op_event_loop
{
    int poll_fd;
    int flags;
    size_t nb_events;
    size_t next_timeout_id;
    SLIST_HEAD(op_event_loop_events, op_event) events_list;
    SLIST_HEAD(op_event_loop_updates, op_event) update_list;
    SLIST_HEAD(op_event_loop_removals, op_event) remove_list;
    SLIST_HEAD(op_event_loop_disables, op_event) disable_list;
};

struct op_event_loop* op_event_loop_allocate(void)
{
    struct op_event_loop* loop = NULL;

    if ((loop = calloc(1, sizeof(*loop))) == NULL) { return (NULL); }

    if ((loop->poll_fd = kqueue()) == -1) {
        free(loop);
        return (NULL);
    }

    SLIST_INIT(&loop->events_list);
    SLIST_INIT(&loop->update_list);
    SLIST_INIT(&loop->remove_list);
    SLIST_INIT(&loop->disable_list);

    loop->next_timeout_id = (size_t)INT_MAX + 1;

    return (loop);
}

void op_event_loop_set_edge_triggered(struct op_event_loop* loop, bool value)
{
    if (value) {
        loop->flags |= OP_EVENT_LOOP_EDGETRIGGER;
    } else {
        loop->flags &= ~OP_EVENT_LOOP_EDGETRIGGER;
    }
}

size_t op_event_loop_count(struct op_event_loop* loop)
{
    return (loop->nb_events);
}

void op_event_loop_free(struct op_event_loop** loop_p)
{
    assert(*loop_p);
    struct op_event_loop* loop = *loop_p;

    /* Remove all registered events */
    op_event_loop_purge(loop);
    assert(loop->nb_events == 0);

    if (loop->poll_fd != -1) { close(loop->poll_fd); }

    free(loop);

    *loop_p = NULL;
}

void op_event_loop_exit(struct op_event_loop* loop)
{
    loop->flags &= ~OP_EVENT_LOOP_RUNNING;
}

int _enqueue_new_event(struct op_event_loop* loop, struct op_event* event)
{
    event->data.loop = loop;
    SLIST_INSERT_HEAD(&loop->events_list, event, events_link);
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    loop->nb_events++;

    return (0);
}

int op_event_loop_add_fd(struct op_event_loop* loop,
                         int fd,
                         const struct op_event_callbacks* callbacks,
                         void* arg)
{
    assert(loop);
    assert(callbacks);

    if (fd < 0 || !(callbacks->on_read || callbacks->on_write)) {
        return (-EINVAL);
    }

    /* Only non-blocking descriptors are allowed here */
    if (op_event_loop_set_nonblocking(fd) != 0) { return (-1); }

    struct op_event* event = calloc(1, sizeof(*event));
    if (!event) { return (-ENOMEM); }

    event->data.type = OP_EVENT_TYPE_FD;
    event->data.fd = fd;
    event->callbacks = *callbacks;
    event->arg = arg;

    return (_enqueue_new_event(loop, event));
}

int op_event_loop_add_zmq(struct op_event_loop* loop,
                          void* socket,
                          const struct op_event_callbacks* callbacks,
                          void* arg)
{
    assert(loop);
    assert(socket);
    assert(callbacks);

    int fd = op_event_loop_get_socket_fd(socket);
    if (fd < 0 || !(callbacks->on_read || callbacks->on_write)) {
        return (-EINVAL);
    }

    struct op_event* event = calloc(1, sizeof(*event));
    if (!event) { return (-ENOMEM); }

    event->data.type = OP_EVENT_TYPE_ZMQ;
    event->data.socket_fd = fd;
    event->data.socket = socket;
    event->callbacks = *callbacks;
    event->arg = arg;

    return (_enqueue_new_event(loop, event));
}

int op_event_loop_add_timer_ided(struct op_event_loop* loop,
                                 uint64_t timeout_ns,
                                 const struct op_event_callbacks* callbacks,
                                 void* arg,
                                 uint32_t* timeout_id)
{
    assert(loop);
    assert(callbacks);

    if (!callbacks->on_timeout) { return (-EINVAL); }

    struct op_event* event = calloc(1, sizeof(*event));
    if (!event) { return (-ENOMEM); }

    event->data.type = OP_EVENT_TYPE_TIMER;
    event->data.timeout_ns = timeout_ns;
    event->data.timeout_id = loop->next_timeout_id++;
    event->callbacks = *callbacks;
    event->arg = arg;

    if (timeout_id) { *timeout_id = event->data.timeout_id; }

    return (_enqueue_new_event(loop, event));
}

struct op_event* op_event_loop_find_fd(struct op_event_loop* loop, int fd)
{
    struct op_event* event = NULL;
    SLIST_FOREACH (event, &loop->events_list, events_link) {
        if (event->data.type == OP_EVENT_TYPE_FD && event->data.fd == fd) {
            break;
        }
    }

    return (event);
}

struct op_event* op_event_loop_find_zmq(struct op_event_loop* loop,
                                        void* socket)
{
    struct op_event* event = NULL;
    SLIST_FOREACH (event, &loop->events_list, events_link) {
        if (event->data.type == OP_EVENT_TYPE_ZMQ
            && event->data.socket == socket) {
            break;
        }
    }
    return (event);
}

struct op_event* op_event_loop_find_timer(struct op_event_loop* loop,
                                          uint32_t timeout_id)
{
    struct op_event* event = NULL;
    SLIST_FOREACH (event, &loop->events_list, events_link) {
        if (event->data.type == OP_EVENT_TYPE_TIMER
            && event->data.timeout_id == timeout_id) {
            break;
        }
    }
    return (event);
}

int op_event_loop_update_fd(struct op_event_loop* loop,
                            int fd,
                            const struct op_event_callbacks* callbacks)
{
    struct op_event* event = op_event_loop_find(loop, fd);
    if (!event) { return (-EINVAL); }

    event->callbacks = *callbacks;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

int op_event_loop_update_zmq(struct op_event_loop* loop,
                             void* socket,
                             const struct op_event_callbacks* callbacks)
{
    struct op_event* event = op_event_loop_find(loop, socket);
    if (!event) { return (-EINVAL); }
    event->callbacks = *callbacks;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

int op_event_loop_update_timer_cb(struct op_event_loop* loop,
                                  uint32_t timeout_id,
                                  const struct op_event_callbacks* callbacks)
{
    struct op_event* event = op_event_loop_find(loop, timeout_id);
    if (!event) { return (-EINVAL); }
    event->callbacks = *callbacks;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

int op_event_loop_update_timer_to(struct op_event_loop* loop,
                                  uint32_t timeout_id,
                                  uint64_t timeout)
{
    struct op_event* event = op_event_loop_find(loop, timeout_id);
    if (!event) { return (-EINVAL); }

    event->data.timeout_ns = timeout;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

static int _event_key(const struct op_event* event)
{
    assert(event->data.type);

    switch (event->data.type) {
    case OP_EVENT_TYPE_FD:
        return (event->data.fd);
    case OP_EVENT_TYPE_ZMQ:
        return (event->data.socket_fd);
    case OP_EVENT_TYPE_TIMER:
        return (event->data.timeout_id);
    default:
        return (-1);
    }
}

/*
 * XXX: The event update function can change a callback to NULL.  If that
 * happens while we have a pending event, we have a problem.  To work around
 * that we use the following internal callback to remove the offending fd
 * from the kqueue.
 *
 * Note: This issue is kqueue specific due to the separate read/write filters
 * per fd.
 */
#define DELETE_KQ_FILTER_CALLBACK(name, field, filter)                         \
    static int _delete_kq_##name##_callback(const struct op_event_data* data,  \
                                            __attribute__((unused)) void* arg) \
    {                                                                          \
        struct kevent kev;                                                     \
        EV_SET(&kev, data->field, filter, EV_DELETE, 0, 0, NULL);              \
        kevent(data->loop->poll_fd, &kev, 1, NULL, 0, NULL);                   \
        return (0);                                                            \
    }

DELETE_KQ_FILTER_CALLBACK(read, fd, EVFILT_READ)
DELETE_KQ_FILTER_CALLBACK(write, fd, EVFILT_WRITE)

#undef DELETE_KQ_FILTER_CALLBACK

static int _disable_event(struct op_event_loop* loop, struct op_event* event)
{
    /* Cancel pending updates and add to disable list */
    SLIST_FIND_AND_REMOVE(&loop->update_list, update_link, op_event, event);
    SLIST_INSERT_IF_MISSING(&loop->disable_list, event, disable_link);

    /* Attempt to remove from kqueue */
    size_t idx = 0;
    struct kevent kevents[2];

    switch (event->data.type) {
    case OP_EVENT_TYPE_TIMER:
        EV_SET(&kevents[idx++],
               _event_key(event),
               EVFILT_TIMER,
               EV_DISABLE,
               0,
               0,
               event);
        break;
    case OP_EVENT_TYPE_ZMQ:
    case OP_EVENT_TYPE_FD:
        if (event->callbacks.on_read
            && event->callbacks.on_read != _delete_kq_read_callback) {
            EV_SET(&kevents[idx++],
                   _event_key(event),
                   EVFILT_READ,
                   EV_DISABLE,
                   0,
                   0,
                   event);
        }

        if (event->callbacks.on_write
            && event->callbacks.on_write != _delete_kq_write_callback) {
            EV_SET(&kevents[idx++],
                   _event_key(event),
                   EVFILT_WRITE,
                   EV_DISABLE,
                   0,
                   0,
                   event);
        }
        break;
    }

    /*
     * Atempt to remove event from kqueue; if kqueue doesn't know about the
     * event yet then that's still ok.
     */
    int error = kevent(loop->poll_fd, kevents, idx, NULL, 0, NULL);
    return (!error || errno == ENOENT ? 0 : -errno);
}

int op_event_loop_disable_fd(struct op_event_loop* loop, int fd)
{
    struct op_event* event = op_event_loop_find(loop, fd);
    if (!event) { return (-EINVAL); }
    return (_disable_event(loop, event));
}

int op_event_loop_disable_zmq(struct op_event_loop* loop, void* socket)
{
    struct op_event* event = op_event_loop_find(loop, socket);
    if (!event) { return (-EINVAL); }
    return (_disable_event(loop, event));
}

int op_event_loop_disable_timer(struct op_event_loop* loop, uint32_t timeout_id)
{
    struct op_event* event = op_event_loop_find(loop, timeout_id);
    if (!event) { return (-EINVAL); }
    return (_disable_event(loop, event));
}

int op_event_loop_enable_all(struct op_event_loop* loop)
{
    size_t idx = 0;
    struct kevent kevents[loop->nb_events * 2];

    while (!SLIST_EMPTY(&loop->disable_list)) {
        struct op_event* event = SLIST_FIRST(&loop->disable_list);
        SLIST_REMOVE_HEAD(&loop->disable_list, disable_link);

        switch (event->data.type) {
        case OP_EVENT_TYPE_TIMER:
            EV_SET(&kevents[idx++],
                   _event_key(event),
                   EVFILT_TIMER,
                   EV_ENABLE,
                   0,
                   0,
                   event);
            break;
        case OP_EVENT_TYPE_ZMQ:
        case OP_EVENT_TYPE_FD:
            if (event->callbacks.on_read
                && event->callbacks.on_read != _delete_kq_read_callback) {
                EV_SET(&kevents[idx++],
                       _event_key(event),
                       EVFILT_READ,
                       EV_ENABLE,
                       0,
                       0,
                       event);
            }

            if (event->callbacks.on_write
                && event->callbacks.on_write != _delete_kq_write_callback) {
                EV_SET(&kevents[idx++],
                       _event_key(event),
                       EVFILT_WRITE,
                       EV_ENABLE,
                       0,
                       0,
                       event);
            }
            break;
        }
    }

    /*
     * Attempt to enable events.  Note that a failure here means that
     * the event is not registered with the kqueue.  In that case, just
     * add the event to our update list so that it can get added when
     * the event loop is (re)started.
     */
    struct kevent kerrors[idx];
    int events = kevent(loop->poll_fd, kevents, idx, kerrors, idx, NULL);
    for (int i = 0; i < events; i++) {
        if (kerrors[i].flags & EV_ERROR) {
            SLIST_INSERT_IF_MISSING(&loop->update_list,
                                    (struct op_event*)kerrors[i].udata,
                                    update_link);
        }
    }
    return (0);
}

int op_event_loop_del_fd(struct op_event_loop* loop, int fd)
{
    struct op_event* event = op_event_loop_find(loop, fd);
    if (!event) { return (-EINVAL); }
    event->flags |= OP_EVENT_DELETED;
    SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    return (0);
}

int op_event_loop_del_zmq(struct op_event_loop* loop, void* socket)
{
    struct op_event* event = op_event_loop_find(loop, socket);
    if (!event) { return (-EINVAL); }
    event->flags |= OP_EVENT_DELETED;
    SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    return (0);
}

int op_event_loop_del_timer(struct op_event_loop* loop, uint32_t timeout_id)
{
    struct op_event* event = op_event_loop_find(loop, timeout_id);
    if (!event) { return (-EINVAL); }
    event->flags |= OP_EVENT_DELETED;
    SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    return (0);
}

size_t _do_event_updates(struct op_event_loop* loop)
{
    size_t idx = 0;
    struct kevent kevents[loop->nb_events * 2];
    uint16_t base_flags =
        (loop->flags & OP_EVENT_LOOP_EDGETRIGGER) ? EV_CLEAR : 0;

    while (!SLIST_EMPTY(&loop->update_list)) {
        struct op_event* event = SLIST_FIRST(&loop->update_list);
        SLIST_REMOVE_HEAD(&loop->update_list, update_link);

        switch (event->data.type) {
        case OP_EVENT_TYPE_TIMER:
            EV_SET(&kevents[idx++],
                   _event_key(event),
                   EVFILT_TIMER,
                   base_flags | EV_ADD,
                   NOTE_NSECONDS,
                   event->data.timeout_ns,
                   event);
            break;
        case OP_EVENT_TYPE_ZMQ:
        case OP_EVENT_TYPE_FD:
            if (event->callbacks.on_read) {
                EV_SET(&kevents[idx++],
                       _event_key(event),
                       EVFILT_READ,
                       base_flags | EV_ADD,
                       0,
                       0,
                       event);
            } else {
                event->callbacks.on_read = _delete_kq_read_callback;
            }

            if (event->callbacks.on_write) {
                EV_SET(&kevents[idx++],
                       _event_key(event),
                       EVFILT_WRITE,
                       base_flags | EV_ADD,
                       0,
                       0,
                       event);
            } else {
                event->callbacks.on_write = _delete_kq_write_callback;
            }
            break;
        }
    }

    if (idx && kevent(loop->poll_fd, kevents, idx, NULL, 0, NULL) == -1) {
        op_exit("Failed to add events to kqueue %d: %s\n",
                loop->poll_fd,
                strerror(errno));
        return (-1);
    }

    return (idx);
}

int _do_event_handling(struct op_event_loop* loop,
                       struct kevent* kevents,
                       size_t nb_events)
{
    for (size_t i = 0; i < nb_events; i++) {
        struct kevent* kev = &kevents[i];
        struct op_event* event = (struct op_event*)kev->udata;

        if (event->flags & OP_EVENT_DELETED) {
            /* Don't call event handlers when event is being removed. */
            continue;
        }

        switch (kev->filter) {
        case EVFILT_READ:
            if (event->data.type == OP_EVENT_TYPE_ZMQ
                && !op_event_loop_get_socket_is_readable(event->data.socket)) {
                continue; /* false alarm */
            }
            if (event->callbacks.on_read(&event->data, event->arg) != 0) {
                SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
            }
            break;
        case EVFILT_WRITE:
            if (event->data.type == OP_EVENT_TYPE_ZMQ
                && !op_event_loop_get_socket_is_writable(event->data.socket)) {
                continue; /* false alarm */
            }
            if (event->callbacks.on_write(&event->data, event->arg) != 0) {
                SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
            }
            break;
        case EVFILT_TIMER:
            if (event->callbacks.on_timeout(&event->data, event->arg) != 0) {
                SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
            }
            break;
        default:
            OP_LOG(OP_LOG_ERROR,
                   "Unsupported kqueue filter type: %d\n",
                   kev->filter);
            SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
            break;
        }

        if (kev->flags & EV_EOF) {
            SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
        }
    }

    return (0);
}

size_t _do_event_removals(struct op_event_loop* loop)
{
    size_t idx = 0;
    struct kevent kevents[loop->nb_events * 2];

    /*
     * XXX: Make two passes through our remove list.
     * The first pass is to remove all events from the kqueue.
     * The second pass is to remove the event from our internal lists
     * and call any close/delete callbacks.  We do it this way because
     * we'll get an error if we try to remove a kqueue filter from a closed
     * file descriptor, and closing the file descriptor is something which the
     * close/delete callbacks might do.
     */
    struct op_event* event = NULL;
    SLIST_FOREACH (event, &loop->remove_list, remove_link) {
        switch (event->data.type) {
        case OP_EVENT_TYPE_TIMER: {
            EV_SET(&kevents[idx++],
                   _event_key(event),
                   EVFILT_TIMER,
                   EV_DELETE,
                   0,
                   0,
                   NULL);
            break;
        }
        case OP_EVENT_TYPE_FD:
        case OP_EVENT_TYPE_ZMQ:
            if (event->callbacks.on_read
                && event->callbacks.on_read != _delete_kq_read_callback) {
                EV_SET(&kevents[idx++],
                       _event_key(event),
                       EVFILT_READ,
                       EV_DELETE,
                       0,
                       0,
                       NULL);
            }

            if (event->callbacks.on_write
                && event->callbacks.on_write != _delete_kq_write_callback) {
                EV_SET(&kevents[idx++],
                       _event_key(event),
                       EVFILT_WRITE,
                       EV_DELETE,
                       0,
                       0,
                       NULL);
            }
        }
    }

    /* Drop all pending events */
    if (idx && kevent(loop->poll_fd, kevents, idx, NULL, 0, NULL) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to delete %zu events from kqueue %d: %s\n",
               idx,
               loop->poll_fd,
               strerror(errno));
    }

    /* Now clean up our lists and run any pending callbacks */
    while (!SLIST_EMPTY(&loop->remove_list)) {
        event = SLIST_FIRST(&loop->remove_list);
        SLIST_REMOVE_HEAD(&loop->remove_list, remove_link);

        if (event->data.type != OP_EVENT_TYPE_TIMER
            && event->callbacks.on_close) {
            event->callbacks.on_close(&event->data, event->arg);
        }

        SLIST_FIND_AND_REMOVE(&loop->update_list, update_link, op_event, event);
        SLIST_FIND_AND_REMOVE(
            &loop->disable_list, disable_link, op_event, event);
        SLIST_REMOVE(&loop->events_list, event, op_event, events_link);

        if (event->callbacks.on_delete) {
            event->callbacks.on_delete(&event->data, event->arg);
        }

        loop->nb_events--;

        free(event);
    }

    return (idx);
}

void op_event_loop_purge(struct op_event_loop* loop)
{
    struct op_event* event = NULL;
    SLIST_FOREACH (event, &loop->events_list, events_link) {
        SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    }
    _do_event_removals(loop);
}

int op_event_loop_run_timeout(struct op_event_loop* loop, int timeout)
{
    loop->flags |= OP_EVENT_LOOP_RUNNING;

    while (loop->flags & OP_EVENT_LOOP_RUNNING) {
        _do_event_updates(loop);

        struct kevent kevents[KQUEUE_MAX_EVENTS];
        struct timespec ts = {}, *tsp = NULL;
        if (timeout != -1) {
            ts = (struct timespec){timeout / 1000, (timeout % 1000) * 1000000};
            tsp = &ts;
        }

        if (loop->nb_events) { /* sleeping with no events could sleep forever */
            int nb_events =
                kevent(loop->poll_fd, NULL, 0, kevents, KQUEUE_MAX_EVENTS, tsp);
            if (nb_events > 0) {
                _do_event_handling(loop, kevents, nb_events);
                _do_event_removals(loop);
            } else {
                /* timeout or error */
                op_event_loop_exit(loop);
                return (nb_events == 0 ? -ETIMEDOUT : -errno);
            }
        }

        if (!loop->nb_events) {
            op_event_loop_exit(loop);
            break;
        }
    }

    return (0);
}

int op_event_loop_run_forever(struct op_event_loop* loop)
{
    return (op_event_loop_run_timeout(loop, -1));
}
