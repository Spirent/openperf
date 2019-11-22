#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include <zmq.h>

#include "core/op_event_loop.h"
#include "core/op_event_loop_utils.h"

/* inline function symbol */
extern int
op_event_loop_add_timer_noid(struct op_event_loop* loop, uint64_t timeout,
                             const struct op_event_callbacks* callbacks,
                             void* arg);

int op_event_loop_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) { return (-errno); }

    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK));
}

int op_event_loop_get_socket_fd(void* socket)
{
    int error = 0;
    int fd = -1;
    size_t fd_size = sizeof(fd);

    if ((error = zmq_getsockopt(socket, ZMQ_FD, &fd, &fd_size)) != 0) {
        return (error);
    }

    assert(sizeof(fd) == fd_size);

    return (fd);
}

/*
 * XXX: For the next two functions, we return true in case there
 * is an error.  The expectation is that the callbacks will handle
 * errors better than the event loop will.
 */
bool op_event_loop_get_socket_is_readable(void* socket)
{
    uint32_t flags = 0;
    size_t flag_size = sizeof(flags);

    int error = zmq_getsockopt(socket, ZMQ_EVENTS, &flags, &flag_size);

    return (error == 0 ? flags & ZMQ_POLLIN : true);
}

bool op_event_loop_get_socket_is_writable(void* socket)
{
    uint32_t flags = 0;
    size_t flag_size = sizeof(flags);

    int error = zmq_getsockopt(socket, ZMQ_EVENTS, &flags, &flag_size);

    return (error == 0 ? flags & ZMQ_POLLOUT : true);
}
