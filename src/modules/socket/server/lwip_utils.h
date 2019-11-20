#ifndef _OP_SOCKET_SERVER_LWIP_UTILS_H_
#define _OP_SOCKET_SERVER_LWIP_UTILS_H_

#include "tl/expected.hpp"
#include "socket/api.h"

struct ip_pcb;
struct tcp_info;
struct tcp_pcb;

namespace openperf {
namespace socket {
namespace server {

tl::expected<socklen_t, int> do_sock_getsockopt(const ip_pcb*,
                                                const api::request_getsockopt&);
tl::expected<void, int>      do_sock_setsockopt(ip_pcb*,
                                                const api::request_setsockopt&);

tl::expected<socklen_t, int> do_ip_getsockopt(const ip_pcb*,
                                              const api::request_getsockopt&);
tl::expected<void, int>      do_ip_setsockopt(ip_pcb*,
                                              const api::request_setsockopt&);

void get_tcp_info(const tcp_pcb*, tcp_info&);

}
}
}

#endif /* _OP_SOCKET_SERVER_LWIP_UTILS_H_ */
