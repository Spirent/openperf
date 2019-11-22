#ifndef _OP_EVENT_LOOP_UTILS_H_
#define _OP_EVENT_LOOP_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

#include <sys/queue.h>

#include "core/op_event_loop.h"

#define SLIST_FIND_AND_REMOVE(head, field, type, tofind) do {     \
        __typeof__ (tofind) current = NULL;                       \
        SLIST_FOREACH(current, head, field) {                     \
            if (current == tofind)                                \
                break;                                            \
        }                                                         \
        if (current)                                              \
            SLIST_REMOVE(head, current, type, field);             \
    } while (/*CONSTCOND*/0)

#define SLIST_INSERT_IF_MISSING(head, elm, field) do {            \
        __typeof__ (elm) current = NULL;                          \
        bool do_add = true;                                       \
        SLIST_FOREACH(current, head, field) {                     \
            if (current == elm) {                                 \
                do_add = false;                                   \
                break;                                            \
            }                                                     \
        }                                                         \
        if (do_add)                                               \
            SLIST_INSERT_HEAD(head, elm, field);                  \
    } while (/*CONSTCOND*/0)

struct op_event;
struct op_event_loop;

struct op_event *op_event_loop_find_fd(struct op_event_loop *loop,
                                         int fd);
struct op_event *op_event_loop_find_zmq(struct op_event_loop *loop,
                                          void *socket);
struct op_event *op_event_loop_find_timer(struct op_event_loop *loop,
                                            uint32_t timeout_id);

#define op_event_loop_find(loop, thing)                \
    _Generic((thing),                                   \
             int: op_event_loop_find_fd,               \
             const int: op_event_loop_find_fd,         \
             void *: op_event_loop_find_zmq,           \
             uint32_t: op_event_loop_find_timer,       \
             const uint32_t: op_event_loop_find_timer  \
        )(loop, thing)

int  op_event_loop_set_nonblocking(int fd);
int  op_event_loop_get_socket_fd(void *socket);
bool op_event_loop_get_socket_is_readable(void *socket);
bool op_event_loop_get_socket_is_writable(void *socket);

#endif /* _OP_EVENT_LOOP_UTILS_H_ */
