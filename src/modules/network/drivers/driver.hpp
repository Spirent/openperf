#ifndef _OP_NETWORK_DRIVER_HPP_
#define _OP_NETWORK_DRIVER_HPP_

#include <sys/socket.h>
#include <memory>

namespace openperf::network::internal::drivers {
class driver
{
public:
    inline driver() = default;
    virtual ~driver() = default;
    driver(const driver&) = delete;
    driver& operator=(const driver&) = delete;

    template <typename T> static std::shared_ptr<driver> instance()
    {
        static std::shared_ptr<driver> s_instance;
        if (s_instance == nullptr) {
            s_instance = std::make_shared<T>();
            s_instance->init();
        }

        return s_instance;
    }

    virtual void init(){};
    virtual bool bind_to_device_required() { return false; };

    /* General socket functions */
    virtual int
    accept(int s, struct sockaddr* addr, socklen_t* addrlen, int flags = 0) = 0;
    virtual int bind(int s, const struct sockaddr* name, socklen_t namelen) = 0;
    virtual int shutdown(int s, int how) = 0;
    virtual int
    getpeername(int s, struct sockaddr* name, socklen_t* namelen) = 0;
    virtual int
    getsockname(int s, struct sockaddr* name, socklen_t* namelen) = 0;
    virtual int getsockopt(
        int s, int level, int optname, void* optval, socklen_t* optlen) = 0;
    virtual int setsockopt(int s,
                           int level,
                           int optname,
                           const void* optval,
                           socklen_t optlen) = 0;
    virtual int close(int s) = 0;
    virtual int
    connect(int s, const struct sockaddr* name, socklen_t namelen) = 0;
    virtual int listen(int s, int backlog) = 0;
    virtual int socket(int domain, int type, int protocol) = 0;
    virtual int fcntl(int fd, int cmd, ...) = 0;

    /* Receive functions */
    virtual ssize_t read(int s, void* mem, size_t len) = 0;
    virtual ssize_t readv(int s, const struct iovec* iov, int iovcnt) = 0;
    virtual ssize_t recv(int s, void* mem, size_t len, int flags) = 0;
    virtual ssize_t recvfrom(int s,
                             void* mem,
                             size_t len,
                             int flags,
                             struct sockaddr* from,
                             socklen_t* fromlen) = 0;
    virtual ssize_t recvmsg(int s, struct msghdr* message, int flags) = 0;

    /* Transmit functions */
    virtual ssize_t send(int s, const void* dataptr, size_t len, int flags) = 0;
    virtual ssize_t sendmsg(int s, const struct msghdr* message, int flags) = 0;
    virtual ssize_t sendto(int s,
                           const void* dataptr,
                           size_t len,
                           int flags,
                           const struct sockaddr* to,
                           socklen_t tolen) = 0;
    virtual ssize_t write(int s, const void* dataptr, size_t len) = 0;
    virtual ssize_t writev(int s, const struct iovec* iov, int iovcnt) = 0;

    virtual unsigned int if_nametoindex(const char* ifname) = 0;
};
using driver_ptr = std::shared_ptr<driver>;

} // namespace openperf::network::internal::drivers

#endif
