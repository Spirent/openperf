#ifndef _OP_SOCKET_SERVER_COMPAT_LINUX_ICMP_H_
#define _OP_SOCKET_SERVER_COMPAT_LINUX_ICMP_H_

#include <stdint.h>

#define LINUX_ICMP_FILTER 1

struct linux_icmp_filter {
    uint32_t data;
};

#endif /* _OP_SOCKET_SERVER_COMPAT_LINUX_ICMP_H_ */
