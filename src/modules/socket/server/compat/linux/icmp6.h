#ifndef _OP_SOCKET_SERVER_COMPAT_LINUX_ICMP6_H_
#define _OP_SOCKET_SERVER_COMPAT_LINUX_ICMP6_H_

#include <stdint.h>

#define LINUX_ICMP6_FILTER 1

struct linux_icmp6_filter
{
    uint32_t data[8];
};

#define LINUX_ICMP6_FILTER_WILLPASS(type, filterp) \
        ((((filterp)->data[(type) >> 5]) & (1 << ((type) & 31))) == 0)

#define LINUX_ICMP6_FILTER_WILLBLOCK(type, filterp) \
        ((((filterp)->data[(type) >> 5]) & (1 << ((type) & 31))) != 0)

#define LINUX_ICMP6_FILTER_SETPASS(type, filterp) \
        ((((filterp)->data[(type) >> 5]) &= ~(1 << ((type) & 31))))

#define LINUX_ICMP6_FILTER_SETBLOCK(type, filterp) \
        ((((filterp)->data[(type) >> 5]) |=  (1 << ((type) & 31))))

#define LINUX_ICMP6_FILTER_SETPASSALL(filterp) \
        memset (filterp, 0, sizeof (struct linux_icmp6_filter));

#define LINUX_ICMP6_FILTER_SETBLOCKALL(filterp) \
        memset (filterp, 0xFF, sizeof (struct linux_icmp6_filter));

#endif /* _OP_SOCKET_SERVER_COMPAT_LINUX_ICMP6_H_ */

