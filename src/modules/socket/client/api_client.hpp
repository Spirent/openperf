#ifndef _OP_SOCKET_API_CLIENT_HPP_
#define _OP_SOCKET_API_CLIENT_HPP_

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unordered_map>
#include <atomic>

#include "socket/api.hpp"
#include "socket/shared_segment.hpp"
#include "socket/unix_socket.hpp"
#include "core/op_uuid.hpp"

#include "channels_hashtab.hpp"

namespace openperf::socket::api {

template <typename T> class thread_singleton
{
public:
    static T& instance()
    {
        static thread_local T instance;
        return instance;
    }

    thread_singleton(const thread_singleton&) = delete;
    thread_singleton& operator=(const thread_singleton) = delete;

protected:
    thread_singleton() = default;
};

class client : public thread_singleton<client>
{

    core::uuid m_uuid;
    unix_socket m_sock;

    std::unique_ptr<memory::shared_segment> m_shm;
    std::atomic_bool* m_init_flag;

public:
    client();
    ~client();

    void init(std::atomic_bool* init_flag);

    bool is_socket(int s);

    /* General socket functions */
    int accept(int s, struct sockaddr* addr, socklen_t* addrlen, int flags = 0);
    int bind(int s, const struct sockaddr* name, socklen_t namelen);
    int shutdown(int s, int how);
    int getpeername(int s, struct sockaddr* name, socklen_t* namelen);
    int getsockname(int s, struct sockaddr* name, socklen_t* namelen);
    int
    getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen);
    int setsockopt(
        int s, int level, int optname, const void* optval, socklen_t optlen);
    int close(int s);
    int connect(int s, const struct sockaddr* name, socklen_t namelen);
    int listen(int s, int backlog);
    int socket(int domain, int type, int protocol);
    int fcntl(int fd, int cmd, ...);
    int ioctl(int fd, unsigned long req, ...);

    /* Receive functions */
    ssize_t read(int s, void* mem, size_t len);
    ssize_t readv(int s, const struct iovec* iov, int iovcnt);
    ssize_t recv(int s, void* mem, size_t len, int flags);
    ssize_t recvfrom(int s,
                     void* mem,
                     size_t len,
                     int flags,
                     struct sockaddr* from,
                     socklen_t* fromlen);
    ssize_t recvmsg(int s, struct msghdr* message, int flags);

    /* Transmit functions */
    ssize_t send(int s, const void* dataptr, size_t len, int flags);
    ssize_t sendmsg(int s, const struct msghdr* message, int flags);
    ssize_t sendto(int s,
                   const void* dataptr,
                   size_t len,
                   int flags,
                   const struct sockaddr* to,
                   socklen_t tolen);
    ssize_t write(int s, const void* dataptr, size_t len);
    ssize_t writev(int s, const struct iovec* iov, int iovcnt);
};

} // namespace openperf::socket::api

#endif /* _OP_SOCKET_API_CLIENT_HPP_ */
