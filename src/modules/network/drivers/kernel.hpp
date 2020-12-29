#ifndef _OP_NETWORK_DRIVER_KERNEL_HPP_
#define _OP_NETWORK_DRIVER_KERNEL_HPP_

#include "driver.hpp"

namespace openperf::network::internal::drivers {

class kernel final : public driver
{
public:
    static constexpr std::string_view key = "kernel";

    kernel() = default;
    ~kernel() = default;

    /* General socket functions */
    int accept(int s,
               struct sockaddr* addr,
               socklen_t* addrlen,
               int flags = 0) override;
    int bind(int s, const struct sockaddr* name, socklen_t namelen) override;
    int shutdown(int s, int how) override;
    int getpeername(int s, struct sockaddr* name, socklen_t* namelen) override;
    int getsockname(int s, struct sockaddr* name, socklen_t* namelen) override;
    int getsockopt(int s,
                   int level,
                   int optname,
                   void* optval,
                   socklen_t* optlen) override;
    int setsockopt(int s,
                   int level,
                   int optname,
                   const void* optval,
                   socklen_t optlen) override;
    int close(int s) override;
    int connect(int s, const struct sockaddr* name, socklen_t namelen) override;
    int listen(int s, int backlog) override;
    int socket(int domain, int type, int protocol) override;
    int fcntl(int fd, int cmd, ...) override;

    /* Receive functions */
    ssize_t read(int s, void* mem, size_t len) override;
    ssize_t readv(int s, const struct iovec* iov, int iovcnt) override;
    ssize_t recv(int s, void* mem, size_t len, int flags) override;
    ssize_t recvfrom(int s,
                     void* mem,
                     size_t len,
                     int flags,
                     struct sockaddr* from,
                     socklen_t* fromlen) override;
    ssize_t recvmsg(int s, struct msghdr* message, int flags) override;

    /* Transmit functions */
    ssize_t send(int s, const void* dataptr, size_t len, int flags) override;
    ssize_t sendmsg(int s, const struct msghdr* message, int flags) override;
    ssize_t sendto(int s,
                   const void* dataptr,
                   size_t len,
                   int flags,
                   const struct sockaddr* to,
                   socklen_t tolen) override;
    ssize_t write(int s, const void* dataptr, size_t len) override;
    ssize_t writev(int s, const struct iovec* iov, int iovcnt) override;

    unsigned int if_nametoindex(const char* ifname) override;
};
} // namespace openperf::network::internal::drivers

#endif
