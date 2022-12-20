#ifndef _OP_PACKET_STACK_DPDK_PACKET_FILTER_HPP_
#define _OP_PACKET_STACK_DPDK_PACKET_FILTER_HPP_

#include <optional>
#include <variant>

#include "packet/bpf/bpf.hpp"

struct rte_mbuf;

namespace openperf::packet::stack::dpdk {

namespace impl {

struct accept_filter
{
    bool operator()(const rte_mbuf*) const noexcept { return (true); }
};

struct bpf_filter
{
    packet::bpf::bpf filter;
    bool operator()(const rte_mbuf*) const noexcept;
};

} // namespace impl

class packet_filter
{
public:
    packet_filter(const std::optional<std::string>&);
    ~packet_filter() = default;

    bool accept(const rte_mbuf* packet) const
    {
        return (std::visit(
            [&](const auto& filter_op) { return (filter_op(packet)); },
            m_filter));
    }

    using filter_strategy = std::variant<impl::accept_filter, impl::bpf_filter>;

private:
    filter_strategy m_filter;
};

} // namespace openperf::packet::stack::dpdk

#endif /* _OP_PACKET_STACK_DPDK_PACKET_FILTER_HPP_ */
