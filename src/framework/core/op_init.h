#ifndef _OP_INIT_H_
#define _OP_INIT_H_

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize core services and run all initialization/registration
 * callbacks.
 * Note: This function will not return on error, but it will log failure
 * messages.
 *
 * @params context
 *   ZeroMQ context for messaging sockets
 */
void op_init(void* context, int argc, char* argv[]);

/**
 * Give modules an opportunity to cleanly shut down.
 * Note: The specified context will be terminated, and hence
 * unusable when this functions returns.
 *
 * @params context
 *   ZeroMQ context for messaging sockets
 */
void op_halt(void* context);

#ifdef __cplusplus
}
#endif

#endif /* _OP_INIT_H_ */
