#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <zmq.h>

#include "core/icp_log.h"
#include "core/icp_common.h"
#include "core/icp_socket.h"

#define ZEROMQ_EXTERNAL_ENDPOINT_REGEX "tcp://"

static __attribute__((const)) bool _is_external_socket(const char *endpoint)
{
    return ((strncmp(endpoint, "tcp://", 6) == 0) ? true : false);
}

void icp_socket_cleanup(void **socket)
{
    if (*socket) {
        zmq_close(*socket);
        *socket = NULL;
    }
}

void icp_socket_close(void *socket)
{
    zmq_close(socket);
}

void *icp_socket_get_server(void *context, int type, const char *endpoint)
{
    assert(context);
    assert(endpoint);

    void *server = zmq_socket(context, type);
    if (!server) {
        if (errno == ETERM)
            return (NULL);
        else
            icp_exit("Failed to create server socket for %s: %s (%d)\n", endpoint,
                     zmq_strerror(errno), errno);
    }

    const int ipv6 = 1;
    if (zmq_setsockopt(server, ZMQ_IPV6, &ipv6, sizeof(ipv6)) != 0) {
        ICP_LOG(ICP_LOG_ERROR, "Failed to enable ZMQ IPv6 on %s: %s\n",
                endpoint, zmq_strerror(errno));
    }

    if (type == ZMQ_REP) {
        const int timeout = 1000;  /* milliseconds */
        if (zmq_setsockopt(server, ZMQ_SNDTIMEO, &timeout, sizeof(timeout)) != 0)
            ICP_LOG(ICP_LOG_WARNING, "Unable to set socket timeout on %s: %s\n",
                    endpoint, zmq_strerror(errno));
    }

    if (zmq_bind(server, endpoint) != 0 && errno != ETERM) {
        zmq_close(server);
        icp_exit("Failed to bind to %s: %s\n",
                 endpoint, zmq_strerror(errno));
    }

    return (server);
}

void *icp_socket_get_client_basic(void *context, int type, const char *endpoint)
{
    return (icp_socket_get_client_identified(context, type, endpoint, NULL));
}

void *icp_socket_get_client_identified(void *context, int type,
                                       const char *endpoint,
                                       const char *identity)
{
    assert(context);
    assert(endpoint);

    void *client = zmq_socket(context, type);
    if (!client) {
       if (errno == ETERM)
            return (NULL);
        else
            icp_exit("Failed to create client socket for %s: %s (%d)\n", endpoint,
                     zmq_strerror(errno), errno);
    }

    if (identity) {
        if (zmq_setsockopt(client, ZMQ_IDENTITY, identity, strlen(identity)) != 0) {
            icp_exit("Failed to set identity %s for %s: %s\n",
                     identity, endpoint,
                     zmq_strerror(errno));
        }
    }

    if (_is_external_socket(endpoint)) {
        const int zero = 0;

        /* Close external sockets immediately when close is called */
        if (zmq_setsockopt(client, ZMQ_LINGER, &zero, sizeof(zero)) != 0) {
            icp_exit("Failed to set linger option for %s: %s\n",
                     endpoint, zmq_strerror(errno));
        }

        const int ipv6 = 1;
        if (zmq_setsockopt(client, ZMQ_IPV6, &ipv6, sizeof(ipv6)) != 0) {
            ICP_LOG(ICP_LOG_ERROR, "Failed to enable ZMQ IPv6 on %s: %s\n",
                    endpoint, zmq_strerror(errno));
        }
    }

    if (zmq_connect(client, endpoint) != 0 && errno != ETERM) {
        zmq_close(client);
        icp_exit("Failed to connect to %s: %s\n",
                 endpoint, zmq_strerror(errno));
    }

    return (client);
}

void *icp_socket_get_client_subscription(void *context,
                                         const char *endpoint,
                                         const char *prefix)
{
    assert(context);
    assert(endpoint);
    assert(prefix);

    void *sub = icp_socket_get_client(context, ZMQ_SUB, endpoint);
    if (!sub)
        return(NULL);

    if (zmq_setsockopt(sub, ZMQ_SUBSCRIBE, prefix, strlen(prefix)) != 0
        && errno != ETERM) {
        zmq_close(sub);
        icp_exit("Failed to add %s subscription to %s: %s\n",
                 prefix, endpoint, zmq_strerror(errno));
    }

    return (sub);
}

int icp_socket_add_subscription(void *socket, const char *key)
{
    assert(socket);
    assert(key);

    return (zmq_setsockopt(socket, ZMQ_SUBSCRIBE, key, strlen(key)));
}

bool icp_socket_has_messages(void *socket)
{
    uint32_t flags = 0;
    size_t flag_size = sizeof(flags);

    int error = zmq_getsockopt(socket, ZMQ_EVENTS, &flags, &flag_size);

    return (error == 0 ? flags & ZMQ_POLLIN
            : errno == ETERM ? true  /* socket context is gone, force a read so client knows */
            : false);
}
