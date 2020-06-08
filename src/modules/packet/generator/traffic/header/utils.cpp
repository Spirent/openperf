#include <functional>

#include "packet/generator/traffic/protocol/all.hpp"
#include "packet/generator/traffic/header/count.hpp"
#include "packet/generator/traffic/header/expand.hpp"
#include "packet/generator/traffic/header/explode.hpp"
#include "packet/generator/traffic/header/utils.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic::header {

template <typename InputIt, typename BinaryOperation>
void adjacent_apply(InputIt cursor, InputIt last, BinaryOperation&& op)
{
    if (cursor == last) return;

    while (++cursor != last) { op(*cursor, *std::prev(cursor)); }
}

static void do_context_update(config_instance& previous,
                              const config_instance& next)
{
    assert(!previous.valueless_by_exception());
    assert(!next.valueless_by_exception());

    const auto context_visitor = [](auto& previous, const auto& next) {
        protocol::update_context(previous.header, next.header);
    };
    std::visit(context_visitor, previous, next);
}

config_container update_context_fields(config_container&& configs)
{
    adjacent_apply(std::make_reverse_iterator(std::end(configs)),
                   std::make_reverse_iterator(std::begin(configs)),
                   [](auto& previous, const auto& next) {
                       do_context_update(previous, next);
                   });

    return (std::move(configs));
}

constexpr auto count_headers_visitor = [](const auto& config) {
    return (count_headers(config));
};

static size_t count_headers_zip(const config_container& configs) noexcept
{
    return (std::accumulate(
        std::begin(configs),
        std::end(configs),
        1UL,
        [](size_t lhs, const auto& rhs) {
            return (std::lcm(lhs, std::visit(count_headers_visitor, rhs)));
        }));
}

static size_t count_headers_cartesian(const config_container& configs) noexcept
{
    return (
        std::accumulate(std::begin(configs),
                        std::end(configs),
                        1UL,
                        [](size_t lhs, const auto& rhs) {
                            return (std::multiplies{}(
                                lhs, std::visit(count_headers_visitor, rhs)));
                        }));
}

size_t count_headers(const config_container& configs, modifier_mux mux) noexcept
{
    assert(mux != modifier_mux::none);

    return (mux == modifier_mux::zip ? count_headers_zip(configs)
                                     : count_headers_cartesian(configs));
}

container make_headers(const config_container& configs,
                       modifier_mux mux) noexcept
{
    auto headers = std::vector<container>{};

    const auto expand_visitor = [](const auto& config) {
        return (expand(config));
    };

    std::transform(std::begin(configs),
                   std::end(configs),
                   std::back_inserter(headers),
                   [&](const auto& config) {
                       return (std::visit(expand_visitor, config));
                   });

    return (explode(headers, mux));
}

template <size_t... N> auto make_dummy_configs(std::index_sequence<N...>)
{
    auto configs = std::vector<config_instance>{};
    (configs.emplace_back(config_instance(std::in_place_index<N>)), ...);
    return (configs);
}

const static auto protocol_configs = make_dummy_configs(
    std::make_index_sequence<std::variant_size_v<config_instance>>{});

using flags = packetio::packet::packet_type::flags;
flags to_packet_type_flags(const config_container& configs) noexcept
{
    auto type_flags = flags{};

    std::for_each(
        std::begin(configs), std::end(configs), [&](const auto& instance) {
            type_flags = std::visit(
                [&](const auto& config) {
                    return (protocol::update_packet_type(type_flags,
                                                         config.header));
                },
                instance);
        });

    return (type_flags);
}

using header_lengths = packetio::packet::header_lengths;
header_lengths
to_packet_header_lengths(const config_container& configs) noexcept
{
    auto lengths = header_lengths{};

    std::for_each(
        std::begin(configs), std::end(configs), [&](const auto& instance) {
            lengths = std::visit(
                [&](const auto& config) {
                    return (protocol::update_header_lengths(lengths,
                                                            config.header));
                },
                instance);
        });

    return (lengths);
}

} // namespace openperf::packet::generator::traffic::header
