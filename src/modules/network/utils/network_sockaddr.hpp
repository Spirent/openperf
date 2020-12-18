#ifndef _OP_NETWORK_INTERNAL_UTILS_HPP_
#define _OP_NETWORK_INTERNAL_UTILS_HPP_

#include <variant>
#include <netinet/in.h>
#include "framework/utils/overloaded_visitor.hpp"

namespace openperf::network::internal {

using network_sockaddr = std::variant<sockaddr_in, sockaddr_in6>;

inline socklen_t get_sa_len(const struct sockaddr* sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
    default:
        return sizeof(struct sockaddr);
    }
}

inline in_port_t get_sa_port(const struct sockaddr* csa)
{
    auto* sa = const_cast<sockaddr*>(csa);
    switch (sa->sa_family) {
    case AF_INET:
        return ((reinterpret_cast<sockaddr_in*>(sa))->sin_port);
    case AF_INET6:
        return ((reinterpret_cast<sockaddr_in6*>(sa))->sin6_port);
    default:
        return (0);
    }
}

inline void* get_sa_addr(const struct sockaddr* csa)
{
    auto* sa = const_cast<sockaddr*>(csa);
    switch (sa->sa_family) {
    case AF_INET:
        return &reinterpret_cast<sockaddr_in*>(sa)->sin_addr;
    case AF_INET6:
        return &reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr;
    default:
        return nullptr;
    }
}

inline tl::expected<network_sockaddr, int>
network_sockaddr_assign(const sockaddr* sa)
{
    switch (sa->sa_family) {
    case AF_INET: {
        sockaddr_in ns;
        memcpy(&ns, sa, get_sa_len(sa));
        return ns;
    }
    case AF_INET6: {
        sockaddr_in6 ns;
        memcpy(&ns, sa, get_sa_len(sa));
        return ns;
    }
    }
    return tl::make_unexpected(EINVAL);
}

inline socklen_t network_sockaddr_size(const network_sockaddr& s)
{
    return std::visit([](auto&& sa) { return sizeof(sa); }, s);
}

inline sa_family_t network_sockaddr_family(const network_sockaddr& s)
{
    return std::visit(
        utils::overloaded_visitor(
            [&](const sockaddr_in& sa) { return sa.sin_family; },
            [&](const sockaddr_in6& sa) { return sa.sin6_family; }),
        s);
}

inline in_port_t network_sockaddr_port(const network_sockaddr& s)
{
    return std::visit(utils::overloaded_visitor(
                          [&](const sockaddr_in& sa) { return sa.sin_port; },
                          [&](const sockaddr_in6& sa) { return sa.sin6_port; }),
                      s);
}

inline void* network_sockaddr_addr(const network_sockaddr& cs)
{
    auto& s = const_cast<network_sockaddr&>(cs);
    return std::visit(utils::overloaded_visitor(
                          [&](sockaddr_in& sa) {
                              return reinterpret_cast<void*>(&sa.sin_addr);
                          },
                          [&](sockaddr_in6& sa) {
                              return reinterpret_cast<void*>(&sa.sin6_addr);
                          }),
                      s);
}

} // namespace openperf::network::internal

#endif
