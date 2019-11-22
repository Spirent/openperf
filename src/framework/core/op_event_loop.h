#ifndef _OP_EVENT_LOOP_H_
#define _OP_EVENT_LOOP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/op_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate an event loop structure.
 *
 * @return
 *   pointer to event loop on success; NULL on failure
 */
struct op_event_loop * op_event_loop_allocate(void);

/**
 * Specify whether interrupts are edge triggered or not.
 * By default, interrupts are level triggered.  Hence, edge triggered
 * mode must be explicitly enabled.
 *
 * @param[in] loop
 *   pointer to event loop
 * @param[in] value
 *   enable/disable edge triggered mode
 */
void op_event_loop_set_edge_triggered(struct op_event_loop *loop, bool value);

/**
 * Return the number of events in the loop
 *
 * @param[in] loop
 *  pointer to event loop
 *
 * @return
 *  the number of events
 */
size_t op_event_loop_count(struct op_event_loop *loop);

/**
 * Clear all events from the loop.
 *
 * @param[in] loop
 *  pointer to event loop
 */
void op_event_loop_purge(struct op_event_loop *loop);

/**
 * Free all resources used by the loop.  This will call all
 * close/delete callbacks.
 *
 * @param[in] loop
 *  address of pointer to loop
 */
void op_event_loop_free(struct op_event_loop **loop);

/**
 * Start the event loop and process events.
 *
 * @param[in, out] loop
 *  pointer to event loop; NULL on return
 *
 * @return
 *  -  0: Success
 *  - !0: Error
 */
int op_event_loop_run_forever(struct op_event_loop *loop);

int op_event_loop_run_timeout(struct op_event_loop *loop, int timeout);

#define GET_LOOP_RUN_MACRO(_1, _2, NAME, ...) NAME
#define op_event_loop_run(...)                                  \
    GET_LOOP_RUN_MACRO(__VA_ARGS__,                             \
                       op_event_loop_run_timeout,               \
                       op_event_loop_run_forever)(__VA_ARGS__)

/**
 * Halt the event loop.  This should be called from a callback
 * running inside the event loop.
 */
void op_event_loop_exit(struct op_event_loop *loop);

/**
 * macro hackery to have a single 'function' for all event additions
 */
int op_event_loop_add_fd(struct op_event_loop *loop,
                         int fd,
                         const struct op_event_callbacks *callbacks,
                         void *arg);

int op_event_loop_add_zmq(struct op_event_loop *loop,
                          void *socket,
                          const struct op_event_callbacks *callbacks,
                          void *arg);

int op_event_loop_add_timer_ided(struct op_event_loop *loop,
                                 uint64_t timeout,
                                 const struct op_event_callbacks *callbacks,
                                 void *arg,
                                 uint32_t *timeout_id);

inline
int op_event_loop_add_timer_noid(struct op_event_loop *loop,
                                 uint64_t timeout,
                                 const struct op_event_callbacks *callbacks,
                                 void *arg)
{
    return (op_event_loop_add_timer_ided(loop, timeout, callbacks, arg, NULL));
}

#define GET_ADD_TIMER_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME
#define op_event_loop_add_timer(...)                                \
    GET_ADD_TIMER_MACRO(__VA_ARGS__,                                \
                        op_event_loop_add_timer_ided,               \
                        op_event_loop_add_timer_noid)(__VA_ARGS__)

/**
 * Macro to add an item to the event loop generically.
 *
 * @param[in] loop
 *  pointer to event loop
 * @param[in] thing
 *  thing to add to the event loop
 * @param[in] callbacks
 *  pointer to filled in callback structure
 * @param[in] arg
 *  pointer to opaque data for callbacks
 *
 * @return
 *  -  0: Success
 *  - !0: Error
 */
#define op_event_loop_add(loop, thing, callbacks, arg)      \
    _Generic((thing),                                       \
             int: op_event_loop_add_fd,                     \
             const int: op_event_loop_add_fd,               \
             void *: op_event_loop_add_zmq,                 \
             uint64_t: op_event_loop_add_timer_noid,        \
             const uint64_t: op_event_loop_add_timer_noid   \
        )(loop, thing, callbacks, arg)

/**
 * macro hackery to have a single 'function' for all event updates
 */
int op_event_loop_update_fd(struct op_event_loop *loop,
                            int fd,
                            const struct op_event_callbacks *callbacks);

int op_event_loop_update_zmq(struct op_event_loop *loop,
                             void *socket,
                             const struct op_event_callbacks *callbacks);

int op_event_loop_update_timer_cb(struct op_event_loop *loop,
                                  uint32_t timeout_id,
                                  const struct op_event_callbacks *callbacks);

int op_event_loop_update_timer_to(struct op_event_loop *loop,
                                  uint32_t timeout_id,
                                  uint64_t timeout);

/**
 * Macro to update an item in the event loop.
 *
 * @param[in] loop
 *  pointer to event loop
 * @param[in] thing
 *  thing to add to the event loop
 * @param[in] arg
 *  updated callbacks or new timeout
 *
 * @return
 *  -  0: Success
 *  - !0: Error
 */
#define op_event_loop_update(loop, thing, arg)                  \
    _Generic((thing),                                           \
             int: op_event_loop_update_fd,                      \
             const int: op_event_loop_update_fd,                \
             void *: op_event_loop_update_zmq,                  \
             uint32_t:                                          \
             _Generic((arg),                                    \
                      struct op_event_callbacks *:              \
                      op_event_loop_update_timer_cb,            \
                      const struct op_event_callbacks *:        \
                      op_event_loop_update_timer_cb,            \
                      uint64_t: op_event_loop_update_timer_to   \
                 )                                              \
        )(loop, thing, arg)

/**
 * macro hackery to have a single 'function' to diable events
 */
int op_event_loop_disable_fd(struct op_event_loop *loop, int fd);
int op_event_loop_disable_zmq(struct op_event_loop *loop, void *socket);
int op_event_loop_disable_timer(struct op_event_loop *loop, uint32_t timeout_id);

/**
 * Macro to disable an event in the loop.
 *
 * @param[in] loop
 *  pointer to event loop
 * @param[in] thing
 *  thing to disable in the event loop
 *
 * @return
 *  -  0: Success
 *  - !0: Error
 */
#define op_event_loop_disable(loop, thing)          \
    _Generic((thing),                               \
             int: op_event_loop_disable_fd,         \
             const int: op_event_loop_disable_fd,   \
             void *: op_event_loop_disable_zmq,     \
             uint32_t: op_event_loop_disable_timer  \
        )(loop, thing)

/**
 * Enable all disabled events.
 *
 * @param[in] loop
 *  pointer to event loop
 *
 * @return
 *  -  0: Success
 *  - !0: Error
 */
int op_event_loop_enable_all(struct op_event_loop *loop);

/**
 * macro hacker to have a single 'function' for all event deletions
 */
int op_event_loop_del_fd(struct op_event_loop *loop, int fd);
int op_event_loop_del_zmq(struct op_event_loop *loop, void *socket);
int op_event_loop_del_timer(struct op_event_loop *loop, uint32_t timeout_id);

/**
 * Macro to delete an event form the event loop.
 *
 * @param[in] loop
 *  pointer to event loop
 * @param[in] thing
 *  thing to remove from the event loop
 *
 * @return
 *  -  0: Success
 *  - !0: Error
 */
#define op_event_loop_del(loop, thing)                  \
    _Generic((thing),                                   \
             int: op_event_loop_del_fd,                 \
             const int: op_event_loop_del_fd,           \
             void *: op_event_loop_del_zmq,             \
             uint32_t: op_event_loop_del_timer,         \
             const uint32_t: op_event_loop_del_timer    \
        )(loop, thing)

#ifdef __cplusplus
}
#endif

#endif /* _OP_EVENT_LOOP_H_ */
