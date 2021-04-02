#ifndef _OP_SOCKET_SERVER_LWIP_UTILS_HPP_
#define _OP_SOCKET_SERVER_LWIP_UTILS_HPP_

#include "tl/expected.hpp"
#include "socket/api.hpp"
#include "lwip/priv/tcpip_priv.h" // required for tcpip_api_call()

struct ip_pcb;
struct tcp_info;
struct tcp_pcb;

namespace openperf::socket::server {

tl::expected<void, int> do_sock_ioctl(const ip_pcb*, const api::request_ioctl&);

tl::expected<socklen_t, int> do_sock_getsockopt(const ip_pcb*,
                                                const api::request_getsockopt&);
tl::expected<void, int> do_sock_setsockopt(ip_pcb*,
                                           const api::request_setsockopt&);

tl::expected<socklen_t, int> do_ip_getsockopt(const ip_pcb*,
                                              const api::request_getsockopt&);
tl::expected<void, int> do_ip_setsockopt(ip_pcb*,
                                         const api::request_setsockopt&);

tl::expected<socklen_t, int> do_ip6_getsockopt(const ip_pcb*,
                                               const api::request_getsockopt&);
tl::expected<void, int> do_ip6_setsockopt(ip_pcb*,
                                          const api::request_setsockopt&);

void get_tcp_info(const tcp_pcb*, tcp_info&);

template <typename Function> class tcpip_api_call_wrapper
{
public:
    tcpip_api_call_wrapper(Function func)
        : m_func(func)
    {}

    static err_t callback(struct tcpip_api_call_data* call_data)
    {
        auto obj = reinterpret_cast<tcpip_api_call_wrapper*>(call_data);
        return obj->m_func();
    }

private:
    Function m_func;
};

/*
 * Execute function in lwip thread.
 * @param func The lambda function to execute.
 * @return err_t The function return value. ERR_OK if success.
 */
template <typename Function> err_t do_tcpip_call(Function func)
{
    using wrapper_type = tcpip_api_call_wrapper<Function>;
    wrapper_type wrapper(func);
    return tcpip_api_call(wrapper_type::callback,
                          reinterpret_cast<tcpip_api_call_data*>(&wrapper));
}

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_LWIP_UTILS_HPP_ */
