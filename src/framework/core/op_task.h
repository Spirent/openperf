#ifndef _OP_TASK_H_
#define _OP_TASK_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OP_TASK_ARGS_NAME_MAX 16

/**
 * Structure providing arguments for tasks
 */
struct op_task_args {
    void *context;                      /**< ZeroMQ context */
    char name[OP_TASK_ARGS_NAME_MAX];  /**< Thread name for task */
    struct {
        const void *args;               /**< (optional) argument for task */
        uint64_t flags;                 /**< (optional) flags for task */
    } custom;
};

typedef void *(task_fn)(void *args);

/**
 * Run task in a new, detached thread.  This task blocks until the new
 * thread has started executing.
 *
 * @param task
 *   The function to run in a new thread
 * @param args
 *   A pointer to an op_task_args structure.
 *   The task function is expected to free this pointer.
 *
 * @return
 *   0: Success
 *  !0: Error
 */
int op_task_launch(task_fn *task,
                    struct op_task_args *task_args,
                    const char *task_name);

/**
 * Open a socket for the purpose of synchronizing with other processes
 *
 * @param context
 *   0MQ context
 * @param endpoint
 *   0MQ endpoint
 *
 * @return
 *   0MQ socket or NULL on error
 */
void *op_task_sync_socket(void *context, const char *endpoint);

/**
 * Wait for responses on the socket.
 * Note: this function always closes the socket
 * @param socketp
 *   address of pointer to sync socket
 * @param expected
 *   expected number of responses to wait for
 *
 * @return
 *   0: Success
 *  !0: Error
 */
int op_task_sync_block(void **socketp, size_t expected);

/**
 * Wait for responses on the socket and log a warning every
 * interval while still waiting on responses.
 *
 * @param socketp
 *  address of pointer to sync socket
 * @param expected
 *  expected number of responses
 * @param interval
 *  polling interval in ms; warnings logged after interval elapses
 * @param format
 *  message warning format
 * @param ...
 *  format arguments
 *
 * @return
 *   0: Success
 *  !0: Error
 */
#define op_task_sync_block_and_warn(socketp, expected,                 \
                                     timeout, format, ...)              \
    _op_task_sync_block_and_warn(socketp, expected, timeout,           \
                                  __func__, format, ##__VA_ARGS__)

int _op_task_sync_block_and_warn(void **socketp, size_t expected,
                                  unsigned interval,
                                  const char *function,
                                  const char *format, ...)
__attribute__((format(printf, 5, 6)));

/**
 * Notify the sync endpoint
 *
 * @param context
 *   0MQ context
 * @param endpoint
 *   0MQ endpoint
 *
 * @return
 *   0: Success
 *  !0: Error
 */
int op_task_sync_ping(void *context, const char *endpoint);

#ifdef __cplusplus
}
#endif

#endif
