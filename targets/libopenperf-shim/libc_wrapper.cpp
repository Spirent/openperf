#include <cerrno>
#include "dlfcn.h"
#include <stdexcept>
#include <string>

#include "libc_wrapper.hpp"

namespace openperf {
namespace socket {
namespace libc {

/**
 * Provide a means to get the correct type for a function without actually
 * knowing what it is.  Sometimes all this C++ machinery is useful. :)
 */
template <typename FunctionPtr>
FunctionPtr load_symbol(void* handle, const char* name)
{
    auto ptr = dlsym(handle, name);
    if (!ptr) {
        throw std::runtime_error("Could not find symbol for "
                                 + std::string(name) + ": "
                                 + std::string(dlerror()));
    }

    return (reinterpret_cast<FunctionPtr>(ptr));
}

void wrapper::init()
{
    dlerror(); /* Make sure there are no pre-existing errors */

    accept = load_symbol<decltype(accept)>(RTLD_NEXT, "accept");
    bind = load_symbol<decltype(bind)>(RTLD_NEXT, "bind");
    shutdown = load_symbol<decltype(shutdown)>(RTLD_NEXT, "shutdown");
    getpeername = load_symbol<decltype(getpeername)>(RTLD_NEXT, "getpeername");
    getsockname = load_symbol<decltype(getsockname)>(RTLD_NEXT, "getsockname");
    getsockopt = load_symbol<decltype(getsockopt)>(RTLD_NEXT, "getsockopt");
    close = load_symbol<decltype(close)>(RTLD_NEXT, "close");
    connect = load_symbol<decltype(connect)>(RTLD_NEXT, "connect");
    listen = load_symbol<decltype(listen)>(RTLD_NEXT, "listen");
    socket = load_symbol<decltype(socket)>(RTLD_NEXT, "socket");
    fcntl = load_symbol<decltype(fcntl)>(RTLD_NEXT, "fcntl");
    ioctl = load_symbol<decltype(ioctl)>(RTLD_NEXT, "ioctl");

    read = load_symbol<decltype(read)>(RTLD_NEXT, "read");
    readv = load_symbol<decltype(readv)>(RTLD_NEXT, "readv");
    recv = load_symbol<decltype(recv)>(RTLD_NEXT, "recv");
    recvfrom = load_symbol<decltype(recvfrom)>(RTLD_NEXT, "recvfrom");
    recvmsg = load_symbol<decltype(recvmsg)>(RTLD_NEXT, "recvmsg");

    send = load_symbol<decltype(send)>(RTLD_NEXT, "send");
    sendmsg = load_symbol<decltype(sendmsg)>(RTLD_NEXT, "sendmsg");
    sendto = load_symbol<decltype(sendto)>(RTLD_NEXT, "sendto");
    setsockopt = load_symbol<decltype(setsockopt)>(RTLD_NEXT, "setsockopt");
    write = load_symbol<decltype(write)>(RTLD_NEXT, "write");
    writev = load_symbol<decltype(writev)>(RTLD_NEXT, "writev");
}

} // namespace libc
} // namespace socket
} // namespace openperf
