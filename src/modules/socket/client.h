#ifndef _ICP_SOCKET_CLIENT_H_
#define _ICP_SOCKET_CLIENT_H_

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unordered_map>

#include "socket/shared_segment.h"
#include "socket/socket_api.h"
#include "socket/unix_socket.h"
#include "socket/uuid.h"

namespace icp {
namespace sock {

template <typename T>
class thread_singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    thread_singleton(const thread_singleton&) = delete;
    thread_singleton& operator=(const thread_singleton) = delete;

protected:
    thread_singleton() {};
};

class client : public thread_singleton<client>
{
    uuid m_uuid;
    unix_socket m_sock;

    struct client_socket {
        struct {
            api::socket_queue* tx;
            api::socket_queue* rx;
        } queue;
        struct {
            int client;
            int server;
        } fd;
        int id;
    };

    /* XXX: need a multi-threaded in case fd's jump threads */
    std::unordered_map<int, client_socket> m_sockets;
    std::unique_ptr<memory::shared_segment> m_shm;

public:
    client();

    void init();
    int socket(int domain, int type, int protocol);
    int close(int s);
    //int write(const char *buffer, size_t length);

    int io_ping(int s);

#if 0
    int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
    int bind(int s, const struct sockaddr *name, socklen_t namelen);
    int shutdown(int s, int how);
    int getpeername(int s, struct sockaddr *name, socklen_t *namelen);
    int getsockname(int s, struct sockaddr *name, socklen_t *namelen);
    int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
    int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
    int close(int s);
    int connect(int s, const struct sockaddr *name, socklen_t namelen);
    int listen(int s, int backlog);
    int recv(int s, void *mem, size_t len, int flags);
    int read(int s, void *mem, size_t len);
    int recvfrom(int s, void *mem, size_t len, int flags,
                 struct sockaddr *from, socklen_t *fromlen);
    int send(int s, const void *dataptr, size_t len, int flags);
    int sendmsg(int s, const struct msghdr *message, int flags);
    int sendto(int s, const void *dataptr, size_t size, int flags);
    int socket(int domain, int type, int protocol);
    int write(int s, const void *dataptr, size_t size);
    int writev(int s, const struct iovec *iov, int iovcnt);

    int ioctl(int s, long cmd, void *argp);
#endif

};

}
}
#endif /* _ICP_SOCKET_CLIENT_H_ */
