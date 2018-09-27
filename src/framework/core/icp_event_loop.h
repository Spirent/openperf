#ifndef _ICP_EVENT_LOOP_H_
#define _ICP_EVENT_LOOP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/icp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate an event loop structure.
 *
 * @return
 *   pointer to event loop on success; NULL on failure
 */
struct icp_event_loop * icp_event_loop_allocate(void);

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
void icp_event_loop_set_edge_triggered(struct icp_event_loop *loop, bool value);

/**
 * Return the number of events in the loop
 *
 * @param[in] loop
 *  pointer to event loop
 *
 * @return
 *  the number of events
 */
size_t icp_event_loop_count(struct icp_event_loop *loop);

/**
 * Clear all events from the loop.
 *
 * @param[in] loop
 *  pointer to event loop
 */
void icp_event_loop_purge(struct icp_event_loop *loop);

/**
 * Free all resources used by the loop.  This will call all
 * close/delete callbacks.
 *
 * @param[in] loop
 *  address of pointer to loop
 */
void icp_event_loop_free(struct icp_event_loop **loop);

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
int icp_event_loop_run_forever(struct icp_event_loop *loop);

int icp_event_loop_run_timeout(struct icp_event_loop *loop, int timeout);

#define GET_LOOP_RUN_MACRO(_1, _2, NAME, ...) NAME
#define icp_event_loop_run(...)                                     \
    GET_LOOP_RUN_MACRO(__VA_ARGS__,                                 \
                       icp_event_loop_run_timeout,                  \
                       icp_event_loop_run_forever)(__VA_ARGS__)

/**
 * Halt the event loop.  This should be called from a callback
 * running inside the event loop.
 */
void icp_event_loop_exit(struct icp_event_loop *loop);

/**
 * macro hackery to have a single 'function' for all event additions
 */
int icp_event_loop_add_fd(struct icp_event_loop *loop,
                          int fd,
                          const struct icp_event_callbacks *callbacks,
                          void *arg);

int icp_event_loop_add_zmq(struct icp_event_loop *loop,
                           void *socket,
                           const struct icp_event_callbacks *callbacks,
                           void *arg);

int icp_event_loop_add_timer_ided(struct icp_event_loop *loop,
                                  uint64_t timeout,
                                  const struct icp_event_callbacks *callbacks,
                                  void *arg,
                                  uint32_t *timeout_id);

inline
int icp_event_loop_add_timer_noid(struct icp_event_loop *loop,
                                  uint64_t timeout,
                                  const struct icp_event_callbacks *callbacks,
                                  void *arg)
{
    return (icp_event_loop_add_timer_ided(loop, timeout, callbacks, arg, NULL));
}

#define GET_ADD_TIMER_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME
#define icp_event_loop_add_timer(...)                               \
    GET_ADD_TIMER_MACRO(__VA_ARGS__,                                \
                        icp_event_loop_add_timer_ided,              \
                        icp_event_loop_add_timer_noid)(__VA_ARGS__)

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
#define icp_event_loop_add(loop, thing, callbacks, arg)         \
    _Generic((thing),                                           \
             int: icp_event_loop_add_fd,                        \
             const int: icp_event_loop_add_fd,                  \
             void *: icp_event_loop_add_zmq,                    \
             uint64_t: icp_event_loop_add_timer_noid,           \
             const uint64_t: icp_event_loop_add_timer_noid      \
        )(loop, thing, callbacks, arg)

/**
 * macro hackery to have a single 'function' for all event updates
 */
int icp_event_loop_update_fd(struct icp_event_loop *loop,
                             int fd,
                             const struct icp_event_callbacks *callbacks);

int icp_event_loop_update_zmq(struct icp_event_loop *loop,
                              void *socket,
                              const struct icp_event_callbacks *callbacks);

int icp_event_loop_update_timer_cb(struct icp_event_loop *loop,
                                   uint32_t timeout_id,
                                   const struct icp_event_callbacks *callbacks);

int icp_event_loop_update_timer_to(struct icp_event_loop *loop,
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
#define icp_event_loop_update(loop, thing, arg)                         \
    _Generic((thing),                                                   \
             int: icp_event_loop_update_fd,                             \
             const int: icp_event_loop_update_fd,                       \
             void *: icp_event_loop_update_zmq,                         \
             uint32_t:                                                  \
             _Generic((arg),                                            \
                      struct icp_event_callbacks *:                     \
                          icp_event_loop_update_timer_cb,               \
                      const struct icp_event_callbacks *:               \
                          icp_event_loop_update_timer_cb,               \
                      uint64_t: icp_event_loop_update_timer_to          \
                 )                                                      \
        )(loop, thing, arg)

/**
 * macro hackery to have a single 'function' to diable events
 */
int icp_event_loop_disable_fd(struct icp_event_loop *loop, int fd);
int icp_event_loop_disable_zmq(struct icp_event_loop *loop, void *socket);
int icp_event_loop_disable_timer(struct icp_event_loop *loop, uint32_t timeout_id);

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
#define icp_event_loop_disable(loop, thing)         \
    _Generic((thing),                               \
             int: icp_event_loop_disable_fd,        \
             const int: icp_event_loop_disable_fd,  \
             void *: icp_event_loop_disable_zmq,    \
             uint32_t: icp_event_loop_disable_timer \
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
int icp_event_loop_enable_all(struct icp_event_loop *loop);

/**
 * macro hacker to have a single 'function' for all event deletions
 */
int icp_event_loop_del_fd(struct icp_event_loop *loop, int fd);
int icp_event_loop_del_zmq(struct icp_event_loop *loop, void *socket);
int icp_event_loop_del_timer(struct icp_event_loop *loop, uint32_t timeout_id);

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
#define icp_event_loop_del(loop, thing)               \
    _Generic((thing),                                 \
             int: icp_event_loop_del_fd,              \
             const int: icp_event_loop_del_fd,        \
             void *: icp_event_loop_del_zmq,          \
             uint32_t: icp_event_loop_del_timer,      \
             const uint32_t: icp_event_loop_del_timer \
        )(loop, thing)

#ifdef __cplusplus
}
#endif

#endif /* _ICP_EVENT_LOOP_H_ */
