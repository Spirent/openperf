#include "packet/generator/traffic/protocol/protocols.hpp"
#include "packet/generator/traffic/header/count.hpp"
#include "packet/generator/traffic/header/expand.hpp"
#include "packet/generator/traffic/header/explode.hpp"
#include "packet/generator/traffic/header/utils.hpp"

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
    auto context_visitor = [](auto& previous, const auto& next) {
        protocol::update_context(previous.header, next.header);
    };
    std::visit(context_visitor, previous, next);
}

config_container update_context_fields(config_container&& configs) noexcept
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

static size_t count_headers_zip(const config_container& configs)
{
    return (std::accumulate(
        std::begin(configs),
        std::end(configs),
        1UL,
        [](size_t lhs, const auto& rhs) {
            return (std::lcm(lhs, std::visit(count_headers_visitor, rhs)));
        }));
}

static size_t count_headers_cartesian(const config_container& configs)
{
    return (std::accumulate(
        std::begin(configs),
        std::end(configs),
        1UL,
        [](size_t lhs, const auto& rhs) {
            return (lhs * std::visit(count_headers_visitor, rhs));
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

void update_lengths(const config_key& indexes,
                    uint8_t packet[],
                    uint16_t pkt_length) noexcept
{
    const static auto configs = make_dummy_configs(
        std::make_index_sequence<std::variant_size_v<config_instance>>{});

    auto cursor = packet;
    auto offset = 0U;

    const auto update_length_visitor = [&](const auto& config) {
        using Protocol = decltype(config.header);
        auto p = reinterpret_cast<Protocol*>(cursor + offset);
        protocol::update_length(*p, pkt_length - offset);
        offset += Protocol::protocol_length;
    };

    std::for_each(std::begin(indexes), std::end(indexes), [&](const auto& idx) {
        std::visit(update_length_visitor, configs[idx]);
    });
}

config_key get_config_key(const config_container& configs) noexcept
{
    /* Turn the configs into a vector of variant indexes */
    auto indexes = config_key{};
    std::transform(std::begin(configs),
                   std::end(configs),
                   std::back_inserter(indexes),
                   [](const auto& config) { return (config.index()); });

    return (indexes);
}

} // namespace openperf::packet::generator::traffic::header
