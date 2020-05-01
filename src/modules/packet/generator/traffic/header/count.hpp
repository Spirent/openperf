#ifndef _OP_PACKET_GENERATOR_TRAFFIC_HEADER_COUNT_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_HEADER_COUNT_HPP_

#include "packet/generator/traffic/header/config.hpp"

namespace openperf::packet::generator::traffic::header {

constexpr auto config_length_visitor = [](const auto& config) {
    return (config.length());
};

template <typename PacketHeader>
size_t count_headers_zip(const config<PacketHeader>& config) noexcept
{
    return (std::accumulate(
        std::begin(config.modifiers),
        std::end(config.modifiers),
        1UL,
        [](size_t lhs, const auto& rhs) {
            return (
                std::lcm(lhs, std::visit(config_length_visitor, rhs.second)));
        }));
}

template <typename PacketHeader>
size_t count_headers_cartesian(const config<PacketHeader>& config) noexcept
{
    return (std::accumulate(
        std::begin(config.modifiers),
        std::end(config.modifiers),
        1UL,
        [](size_t lhs, const auto& rhs) {
            return (std::multiplies{}(
                lhs, std::visit(config_length_visitor, rhs.second)));
        }));
}

template <typename PacketHeader>
size_t count_headers(const config<PacketHeader>& config) noexcept
{
    assert(!config.modifiers.empty());

    /*
     * If we only have one modifier, then we can just return the length
     * of the modifier.
     */
    if (config.modifiers.size() == 1) {
        return (std::visit(config_length_visitor, config.modifiers[0].second));
    }

    /* Else, we need to handle each muxing case appropriately. */
    assert(config.mux != modifier_mux::none);

    return (config.mux == modifier_mux::zip ? count_headers_zip(config)
                                            : count_headers_cartesian(config));
}

} // namespace openperf::packet::generator::traffic::header

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_HEADER_COUNT_HPP_ */
