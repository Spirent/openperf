#ifndef _ICP_SOCKET_H_
#define _ICP_SOCKET_H_

#include <stdbool.h>

void icp_socket_cleanup(void **s);
#define SCOPED_ZMQ_SOCKET(s) void *s __attribute__((cleanup(icp_socket_cleanup)))

/**
 * Provide a zmq close socket function sharing the same signature as free
 */
void icp_socket_close(void *s);

void *icp_socket_get_server(void *context, int type, const char *endpoint);
void *icp_socket_get_client_basic(void *context, int type, const char *endpoint);
void *icp_socket_get_client_identified(void *context,
                                       int type,
                                       const char *endpoint,
                                       const char *identity);
void *icp_socket_get_client_subscription(void *context,
                                         const char *endpoint,
                                         const char *prefix);

/* XXX: _1 and _2 are just placeholders.  No function actually matches. */
#define GET_CLIENT_SOCKET_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define icp_socket_get_client(...)                                    \
    GET_CLIENT_SOCKET_MACRO(__VA_ARGS__,                              \
                            icp_socket_get_client_identified,         \
                            icp_socket_get_client_basic)(__VA_ARGS__)

int icp_socket_add_subscription(void *socket, const char *key);

bool icp_socket_has_messages(void *socket);

#endif
