#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>

#include <zmq.h>

#include "core/icp_common.h"
#include "core/icp_event_loop.h"
#include "core/icp_event_loop_utils.h"
#include "core/icp_log.h"

const uint64_t NS_PER_SECOND = 1000000000ULL;

static const size_t EPOLL_MAX_EVENTS = 1024;

enum icp_event_type {
    ICP_EVENT_TYPE_NONE = 0,
    ICP_EVENT_TYPE_FD,
    ICP_EVENT_TYPE_ZMQ,
    ICP_EVENT_TYPE_TIMER,
    ICP_EVENT_TYPE_FILE,
};

struct icp_event {
    struct icp_event_data data;
    struct icp_event_callbacks callbacks;
    void *arg;
    SLIST_ENTRY(icp_event) events_link;
    SLIST_ENTRY(icp_event) update_link;
    SLIST_ENTRY(icp_event) remove_link;
    SLIST_ENTRY(icp_event) disable_link;
    SLIST_ENTRY(icp_event) always_link;
};

#define ICP_EVENT_LOOP_RUNNING     (1 << 0)
#define ICP_EVENT_LOOP_EDGETRIGGER (1 << 1)

struct icp_event_loop {
    int poll_fd;
    int flags;
    size_t nb_events;
    size_t nb_epoll_events;
    SLIST_HEAD(icp_event_loop_events,   icp_event) events_list;
    SLIST_HEAD(icp_event_loop_updates,  icp_event) update_list;
    SLIST_HEAD(icp_event_loop_removals, icp_event) remove_list;
    SLIST_HEAD(icp_event_loop_disables, icp_event) disable_list;
    SLIST_HEAD(icp_event_loop_always,   icp_event) always_list;
};

struct icp_event_loop * icp_event_loop_allocate(void)
{
    struct icp_event_loop *loop = NULL;

    if ((loop = calloc(1, sizeof(*loop))) == NULL) {
        return (NULL);
    }

    if ((loop->poll_fd = epoll_create(1)) == -1) {
        free(loop);
        return (NULL);
    }

    SLIST_INIT(&loop->events_list);
    SLIST_INIT(&loop->update_list);
    SLIST_INIT(&loop->remove_list);
    SLIST_INIT(&loop->disable_list);
    SLIST_INIT(&loop->always_list);

    return (loop);
}

void icp_event_loop_set_edge_triggered(struct icp_event_loop *loop, bool value)
{
    if (value) {
        loop->flags |= ICP_EVENT_LOOP_EDGETRIGGER;
    } else {
        loop->flags &= ~ICP_EVENT_LOOP_EDGETRIGGER;
    }
}

size_t icp_event_loop_count(struct icp_event_loop *loop)
{
    return (loop->nb_events);
}

void icp_event_loop_free(struct icp_event_loop **loop_p)
{
    assert(*loop_p);
    struct icp_event_loop *loop = *loop_p;

    /* Remove all registered events */
    icp_event_loop_purge(loop);
    assert(loop->nb_events == 0);
    assert(loop->nb_epoll_events == 0);

    if (loop->poll_fd != -1) {
        close(loop->poll_fd);
    }

    free(loop);

    *loop_p = NULL;
}

void icp_event_loop_exit(struct icp_event_loop *loop)
{
    loop->flags &= ~ICP_EVENT_LOOP_RUNNING;
}

static int _enqueue_new_event(struct icp_event_loop *loop,
                              struct icp_event *event)
{
    event->data.loop = loop;
    SLIST_INSERT_HEAD(&loop->events_list, event, events_link);
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    loop->nb_events++;

    return (0);
}

/*
 * Query the underlying file type as Linux requires special handling
 * for proper files.
 */
enum icp_event_type _get_fd_event_type(int fd)
{
    struct stat sb = {};
    if (fstat(fd, &sb) == -1) {
        return (ICP_EVENT_TYPE_NONE);
    }

    return ((S_ISBLK(sb.st_mode) || S_ISREG(sb.st_mode))
            ? ICP_EVENT_TYPE_FILE : ICP_EVENT_TYPE_FD);
}

int icp_event_loop_add_fd(struct icp_event_loop *loop,
                          int fd,
                          const struct icp_event_callbacks *callbacks,
                          void *arg)
{
    assert(loop);
    assert(callbacks);

    if (fd < 0 || !(callbacks->on_read || callbacks->on_write)) {
        return (-EINVAL);
    }

    /* Only non-blocking descriptors are allowed here */
    if (icp_event_loop_set_nonblocking(fd) != 0) {
        return (-1);
    }

    struct icp_event *event = calloc(1, sizeof(*event));
    if (!event) {
        return (-ENOMEM);
    }

    if ((event->data.type = _get_fd_event_type(fd)) == ICP_EVENT_TYPE_NONE) {
        free(event);
        return (-errno);
    }
    event->data.fd = fd;
    event->callbacks = *callbacks;
    event->arg = arg;

    return (_enqueue_new_event(loop, event));
}

int icp_event_loop_add_zmq(struct icp_event_loop *loop,
                           void *socket,
                           const struct icp_event_callbacks *callbacks,
                           void *arg)
{
    assert(loop);
    assert(socket);
    assert(callbacks);

    int fd = icp_event_loop_get_socket_fd(socket);
    if (fd < 0 || !(callbacks->on_read || callbacks->on_write)) {
        return (-EINVAL);
    }

    struct icp_event *event = calloc(1, sizeof(*event));
    if (!event) {
        return (-ENOMEM);
    }

    event->data.type = ICP_EVENT_TYPE_ZMQ;
    event->data.socket_fd = fd;
    event->data.socket = socket;
    event->callbacks = *callbacks;
    event->arg = arg;

    return (_enqueue_new_event(loop, event));
}

int icp_event_loop_add_timer_ided(struct icp_event_loop *loop,
                                  uint64_t timeout_ns,
                                  const struct icp_event_callbacks *callbacks,
                                  void *arg,
                                  uint32_t *timeout_id)
{
    assert(loop);
    assert(callbacks);

    if (!callbacks->on_timeout) {
        return (-EINVAL);
    }

    struct icp_event *event = calloc(1, sizeof(*event));
    if (!event) {
        return (-ENOMEM);
    }

    if ((event->data.timeout_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) == -1) {
        free(event);
        return (-errno);
    }

    event->data.type = ICP_EVENT_TYPE_TIMER;
    event->data.timeout_ns = timeout_ns;
    event->callbacks = *callbacks;
    event->arg = arg;

    if (timeout_id) {
        *timeout_id = event->data.timeout_id;
    }

    return (_enqueue_new_event(loop, event));
}

struct icp_event *icp_event_loop_find_fd(struct icp_event_loop *loop, int fd)
{
    enum icp_event_type type = _get_fd_event_type(fd);
    if (type == ICP_EVENT_TYPE_NONE) {
        return (NULL);
    }

    struct icp_event *event = NULL;
    SLIST_FOREACH(event, &loop->events_list, events_link) {
        if ((enum icp_event_type)event->data.type == type && event->data.fd == fd) {
            break;
        }
    }

    return (event);
}

struct icp_event * icp_event_loop_find_zmq(struct icp_event_loop *loop,
                                           void *socket)
{
    struct icp_event *event = NULL;
    SLIST_FOREACH(event, &loop->events_list, events_link) {
        if (event->data.type == ICP_EVENT_TYPE_ZMQ
            && event->data.socket == socket) {
            break;
        }
    }
    return (event);
}

struct icp_event * icp_event_loop_find_timer(struct icp_event_loop *loop,
                                             uint32_t timeout_id)
{
    struct icp_event *event = NULL;
    SLIST_FOREACH(event, &loop->events_list, events_link) {
        if (event->data.type == ICP_EVENT_TYPE_TIMER
            && event->data.timeout_id == timeout_id) {
            break;
        }
    }
    return (event);
}

int icp_event_loop_update_fd(struct icp_event_loop *loop,
                             int fd,
                             const struct icp_event_callbacks *callbacks)
{
    struct icp_event *event = icp_event_loop_find(loop, fd);
    if (!event) {
        return (-EINVAL);
    }

    event->callbacks = *callbacks;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

int icp_event_loop_update_zmq(struct icp_event_loop *loop,
                              void *socket,
                              const struct icp_event_callbacks *callbacks)
{
    struct icp_event *event = icp_event_loop_find(loop, socket);
    if (!event) {
        return (-EINVAL);
    }
    event->callbacks = *callbacks;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

int icp_event_loop_update_timer_cb(struct icp_event_loop *loop,
                                   uint32_t timeout_id,
                                   const struct icp_event_callbacks *callbacks)
{
    struct icp_event *event = icp_event_loop_find(loop, timeout_id);
    if (!event) {
        return (-EINVAL);
    }
    event->callbacks = *callbacks;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

int icp_event_loop_update_timer_to(struct icp_event_loop *loop,
                                   uint32_t timeout_id,
                                   uint64_t timeout)
{
    struct icp_event *event = icp_event_loop_find(loop, timeout_id);
    if (!event) {
        return (-EINVAL);
    }

    event->data.timeout_ns = timeout;
    SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    return (0);
}

static int _event_key(const struct icp_event *event)
{
    assert(event->data.type);

    switch (event->data.type) {
    case ICP_EVENT_TYPE_FD:
    case ICP_EVENT_TYPE_FILE:
        return (event->data.fd);
    case ICP_EVENT_TYPE_ZMQ:
        return (event->data.socket_fd);
    case ICP_EVENT_TYPE_TIMER:
        return (event->data.timeout_fd);
    default:
        return (-1);
    }
}

static int _disable_event(struct icp_event_loop *loop, struct icp_event *event)
{
    /*
     * Disabling an event simply adds the event to our disable list,
     * cancels any pending updates, and removes the event from epoll.
     */
    SLIST_FIND_AND_REMOVE(&loop->update_list, update_link, icp_event, event);
    SLIST_INSERT_IF_MISSING(&loop->disable_list, event, disable_link);
    int error = epoll_ctl(loop->poll_fd, EPOLL_CTL_DEL, _event_key(event), NULL);
    loop->nb_epoll_events -= !error;

    /* epoll might not know about the event yet, and that's ok */
    return (!error || errno == ENOENT ? 0 : -errno);
}

int icp_event_loop_disable_fd(struct icp_event_loop *loop, int fd)
{
    struct icp_event *event = icp_event_loop_find(loop, fd);
    if (!event) {
        return (-EINVAL);
    }
    return (_disable_event(loop, event));
}

int icp_event_loop_disable_zmq(struct icp_event_loop *loop, void *socket)
{
    struct icp_event *event = icp_event_loop_find(loop, socket);
    if (!event) {
        return (-EINVAL);
    }
    return (_disable_event(loop, event));
}

int icp_event_loop_disable_timer(struct icp_event_loop *loop, uint32_t timeout_id)
{
    struct icp_event *event = icp_event_loop_find(loop, timeout_id);
    if (!event) {
        return (-EINVAL);
    }
    return (_disable_event(loop, event));
}

int icp_event_loop_enable_all(struct icp_event_loop *loop) {
    while (!SLIST_EMPTY(&loop->disable_list)) {
        struct icp_event *event = SLIST_FIRST(&loop->disable_list);
        SLIST_REMOVE_HEAD(&loop->disable_list, disable_link);
        SLIST_INSERT_HEAD(&loop->update_list, event, update_link);
    }
    return (0);
}

int icp_event_loop_del_fd(struct icp_event_loop *loop, int fd)
{
    struct icp_event *event = icp_event_loop_find(loop, fd);
    if (!event) {
        return (-EINVAL);
    }
    SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    return (0);
}

int icp_event_loop_del_zmq(struct icp_event_loop *loop, void *socket)
{
    struct icp_event *event = icp_event_loop_find(loop, socket);
    if (!event) {
        return (-EINVAL);
    }
    SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    return (0);
}

int icp_event_loop_del_timer(struct icp_event_loop *loop, uint32_t timeout_id)
{
    struct icp_event *event = icp_event_loop_find(loop, timeout_id);
    if (!event) {
        return (-EINVAL);
    }
    SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    return (0);
}

static int _event_timer_read(const struct icp_event_data *data)
{
    uint64_t nb_expirations = 0;
    int read_or_err = read(data->timeout_fd, &nb_expirations, sizeof(nb_expirations));
    return (read_or_err == sizeof(nb_expirations) ? 0 : -1);
}


int _epoll_add_or_mod(struct icp_event_loop *loop, int fd, struct epoll_event *event)
{
    int error = epoll_ctl(loop->poll_fd, EPOLL_CTL_MOD, fd, event);
    if (error == -1 && errno == ENOENT) {
        error = epoll_ctl(loop->poll_fd, EPOLL_CTL_ADD, fd, event);
        loop->nb_epoll_events += !error;
    }
    return (error);
}

size_t _do_event_updates(struct icp_event_loop *loop)
{
    size_t nb_events = 0;
    uint32_t base_events = (loop->flags & ICP_EVENT_LOOP_EDGETRIGGER) ? EPOLLRDHUP | EPOLLET : 0;

    while (!SLIST_EMPTY(&loop->update_list)) {
        struct icp_event *event = SLIST_FIRST(&loop->update_list);
        SLIST_REMOVE_HEAD(&loop->update_list, update_link);

        switch (event->data.type) {
        case ICP_EVENT_TYPE_TIMER:
        {
            struct itimerspec its = {};
            its.it_interval.tv_sec = its.it_value.tv_sec =
                event->data.timeout_ns / NS_PER_SECOND;
            its.it_interval.tv_nsec = its.it_value.tv_nsec =
                event->data.timeout_ns % NS_PER_SECOND;

            if (timerfd_settime(_event_key(event), 0, &its, NULL) != 0) {
                icp_exit("Could not set timer: %s\n", strerror(errno));
            }

            struct epoll_event epevent = {
                .events = base_events | EPOLLIN,
                .data = { .ptr = event }
            };

            int error = _epoll_add_or_mod(loop, _event_key(event), &epevent);
            if (error != 0) {
                SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
                ICP_LOG(ICP_LOG_WARNING, "Failed to update timer fd %d in epoll %d: %s\n",
                        _event_key(event), loop->poll_fd, strerror(errno));
            } else {
                nb_events++;
            }
            break;
        }
        case ICP_EVENT_TYPE_FD:
        case ICP_EVENT_TYPE_ZMQ:
        {
            uint32_t events = base_events;
            if (event->callbacks.on_read)
                events |= EPOLLIN;
            if (event->callbacks.on_write)
                events |= EPOLLOUT;

            struct epoll_event epevent = {
                .events = events,
                .data = { .ptr = event }
            };
            int error = _epoll_add_or_mod(loop, _event_key(event), &epevent);
            if (error != 0) {
                SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
                ICP_LOG(ICP_LOG_WARNING, "Failed to update fd %d in epoll %d: %s\n",
                        _event_key(event), loop->poll_fd, strerror(errno));
            } else {
                nb_events++;
            }
            break;
        }
        case ICP_EVENT_TYPE_FILE:
            /*
             * Per the POSIX standard, IEEE Std 1003.1-2008, 2016 Edition
             * ... Regular files shall always poll TRUE for reading and writing ...
             * Unfortunately, Linux doesn't get this right so we have to intervene.
             */
            SLIST_INSERT_IF_MISSING(&loop->always_list, event, always_link);
            break;
        default:
            assert(false);
        }
    }
    return nb_events;
}

int _do_event_handling(struct icp_event_loop *loop,
                       struct epoll_event *epevents, size_t nb_events)
{
    for (size_t i = 0; i < nb_events; i++) {
        struct epoll_event *epev = &epevents[i];
        struct icp_event *event = (struct icp_event *)epev->data.ptr;

        if (epev->events & EPOLLIN) {
            switch (event->data.type) {
            case ICP_EVENT_TYPE_TIMER:
                if (event->callbacks.on_timeout(&event->data, event->arg) != 0) {
                    SLIST_INSERT_IF_MISSING(&loop->remove_list, event,
                                            remove_link);
                } else {
                    /* timer_fd's need to be read in order to be cleared */
                    _event_timer_read(&event->data);
                }
                break;
            case ICP_EVENT_TYPE_ZMQ:
                if (!icp_event_loop_get_socket_is_readable(event->data.socket))
                    break;  /* fall through if readable */
            case ICP_EVENT_TYPE_FD:
                if (event->callbacks.on_read(&event->data, event->arg) != 0) {
                    SLIST_INSERT_IF_MISSING(&loop->remove_list, event,
                                            remove_link);
                }
                break;
            default:
                break;
            }
        }

        if (epev->events & EPOLLOUT) {
            switch (event->data.type) {
            case ICP_EVENT_TYPE_TIMER:
                assert(false);  /* timers are never writable */
            case ICP_EVENT_TYPE_ZMQ:
                if (!icp_event_loop_get_socket_is_writable(event->data.socket))
                    break;  /* fall through if writable */
            case ICP_EVENT_TYPE_FD:
                if (event->callbacks.on_write(&event->data, event->arg) != 0) {
                    SLIST_INSERT_IF_MISSING(&loop->remove_list, event,
                                            remove_link);
                }
                break;
            default:
                break;
            }
        }

        if (epev->events & EPOLLHUP
            || epev->events & EPOLLRDHUP
            || epev->events & EPOLLERR) {
            SLIST_INSERT_IF_MISSING(&loop->remove_list,
                                   event, remove_link);
        }
    }

    return (0);
}

int _do_always_handling(struct icp_event_loop *loop)
{
    /* Handle our _always_ events */
    struct icp_event *event = NULL;
    SLIST_FOREACH(event, &loop->always_list, always_link) {
        if (event->callbacks.on_read
            && event->callbacks.on_read(&event->data, event->arg) != 0) {
            SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
        }

        if (event->callbacks.on_write
            && event->callbacks.on_write(&event->data, event->arg) != 0) {
            SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
        }
    }

    return (0);
}

int _do_event_removals(struct icp_event_loop *loop)
{
    int nb_removals = 0;

    while (!SLIST_EMPTY(&loop->remove_list)) {
        struct icp_event *event = SLIST_FIRST(&loop->remove_list);
        SLIST_REMOVE_HEAD(&loop->remove_list, remove_link);

        if (event->data.type != ICP_EVENT_TYPE_FILE) {
            int error = epoll_ctl(loop->poll_fd, EPOLL_CTL_DEL, _event_key(event), NULL);
            loop->nb_epoll_events -= !error;
        } else {
            SLIST_FIND_AND_REMOVE(&loop->always_list, always_link, icp_event, event);
        }

        SLIST_FIND_AND_REMOVE(&loop->update_list, update_link, icp_event, event);
        SLIST_FIND_AND_REMOVE(&loop->disable_list, disable_link, icp_event, event);
        SLIST_REMOVE(&loop->events_list, event, icp_event, events_link);

        if (event->data.type == ICP_EVENT_TYPE_TIMER) {
            close(event->data.timeout_fd);
        } else if (event->callbacks.on_close) {
            event->callbacks.on_close(&event->data, event->arg);
        }

        if (event->callbacks.on_delete) {
            event->callbacks.on_delete(&event->data, event->arg);
        }

        loop->nb_events--;

        free(event);

        nb_removals++;
    }

    return (nb_removals);
}

void icp_event_loop_purge(struct icp_event_loop *loop)
{
    struct icp_event *event = NULL;
    SLIST_FOREACH(event, &loop->events_list, events_link) {
        SLIST_INSERT_IF_MISSING(&loop->remove_list, event, remove_link);
    }
    _do_event_removals(loop);
}

int icp_event_loop_run_timeout(struct icp_event_loop *loop, int timeout)
{
    loop->flags |= ICP_EVENT_LOOP_RUNNING;

    /*
     * The event loop for epoll is annoyingly complicated; purely because
     * we have to handle proper files which can't be used with epoll.
     */
    while (loop->flags & ICP_EVENT_LOOP_RUNNING) {
        _do_event_updates(loop);

        if (!SLIST_EMPTY(&loop->always_list)) {
            _do_always_handling(loop);
            _do_event_removals(loop);
        }

        if (!loop->nb_events) {
            /* no events at all; abort loop */
            icp_event_loop_exit(loop);
            break;
        }

        if (loop->nb_epoll_events) {
            int epoll_timeout = (SLIST_EMPTY(&loop->always_list) ? timeout : 0);
            struct epoll_event events[EPOLL_MAX_EVENTS];
            int nb_events = epoll_wait(loop->poll_fd, events, EPOLL_MAX_EVENTS, epoll_timeout);
            if (nb_events > 0) {
                _do_event_handling(loop, events, nb_events);
                _do_event_removals(loop);
            } else if (nb_events < 0) {
                /* epoll error */
                icp_event_loop_exit(loop);
                return (-errno);
            } else if (SLIST_EMPTY(&loop->always_list)) {
                /* no ready epoll events and empty always list; must be a timeout */
                icp_event_loop_exit(loop);
                return (-ETIMEDOUT);
            }
        } else if (SLIST_EMPTY(&loop->always_list) && timeout > 0) {
            /* no enabled epoll or always events and a timeout... so timeout? */
            icp_event_loop_exit(loop);
            return (-ETIMEDOUT);
        }
    }

    return (0);
}

int icp_event_loop_run_forever(struct icp_event_loop *loop)
{
    return (icp_event_loop_run_timeout(loop, -1));
}
