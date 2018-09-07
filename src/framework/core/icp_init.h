#ifndef _ICP_INIT_H_
#define _ICP_INIT_H_

/**
 * Initialize core services and run all initialization/registration
 * callbacks.
 * Note: This function will not return on error.
 *
 * @params context
 *   ZeroMQ context for messaging sockets
 */
void icp_init(void *context, int argc, char *argv[]);

#endif /* _ICP_INIT_H_ */
