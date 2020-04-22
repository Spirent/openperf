#ifndef _OP_PACKET_GENERATOR_TRAFFIC_MODIFIER_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_MODIFIER_HPP_

#include <cstdint>
#include <optional>
#include <variant>

#include "range/v3/all.hpp"

#include "lib/packet/type/ipv4_address.hpp"
#include "lib/packet/type/ipv6_address.hpp"
#include "lib/packet/type/mac_address.hpp"

namespace openperf::packet::generator::traffic::modifier {

template <typename Field> struct list_config
{
    std::vector<Field> items;

    size_t length() const noexcept { return (items.size()); }
};

template <typename Field> struct sequence_config
{
    std::vector<Field> skip;
    Field first = Field{0};
    std::optional<Field> last = std::nullopt;
    unsigned count = 0;

    size_t length() const noexcept { return (count); }
};

namespace detail {

template <typename T> struct is_endian_field : std::false_type
{};

template <size_t Octets>
struct is_endian_field<::libpacket::type::endian::field<Octets>>
    : std::true_type
{};

template <typename T> struct is_sequence_config : std::false_type
{};

template <typename Field>
struct is_sequence_config<sequence_config<Field>> : std::true_type
{};

template <typename EndianField>
EndianField
sequence_config_field_step(const sequence_config<EndianField>& config)
{
    static_assert(is_endian_field<EndianField>::value);
    static_assert(EndianField::width <= 4);

    if (config.last) {
        auto last = config.last->template load<uint32_t>();
        auto first = config.first.template load<uint32_t>();
        return (static_cast<uint32_t>((last - first) / config.count));
    }

    return (1);
}

template <typename AddressField>
AddressField
sequence_config_address_step(const sequence_config<AddressField>& config)
{
    static_assert(!is_endian_field<AddressField>::value);
    return (config.last ? (*config.last - config.first) / config.count : 1);
}

template <typename Field> auto to_list_range(const list_config<Field>& config)
{
    return (ranges::views::all(config.items));
}

template <typename EndianField>
auto to_sequence_field_range(const sequence_config<EndianField>& config)
{
    static_assert(is_endian_field<EndianField>::value);
    static_assert(EndianField::width <= 4);

    auto iota = ranges::views::iota(0U, config.count + config.skip.size());

    auto transform = ranges::views::transform(
        [&, step = sequence_config_field_step(config)](unsigned idx) {
            auto value = config.first.template load<uint32_t>()
                         + (step.template load<uint32_t>() * idx);
            return (EndianField(static_cast<uint32_t>(value)));
        });

    auto filter = ranges::views::filter([&](const auto& input) {
        return (
            std::none_of(std::begin(config.skip),
                         std::end(config.skip),
                         [&](const auto& skip) { return (input == skip); }));
    });

    return (iota | transform | filter);
}

template <typename AddressField>
auto to_sequence_address_range(const sequence_config<AddressField>& config)
{
    static_assert(!is_endian_field<AddressField>::value);

    auto iota = ranges::views::iota(0U, config.count + config.skip.size());

    auto transform = ranges::views::transform(
        [&, step = sequence_config_address_step(config)](unsigned idx) {
            return (config.first + (step * idx));
        });

    auto filter = ranges::views::filter([&](const auto& input) {
        return (
            std::none_of(std::begin(config.skip),
                         std::end(config.skip),
                         [&](const auto& skip) { return (input == skip); }));
    });

    return (iota | transform | filter);
}

} // namespace detail

template <typename Config> auto to_range(const Config& config)
{
    if constexpr (detail::is_sequence_config<Config>::value) {
        if constexpr (detail::is_endian_field<decltype(
                          Config().first)>::value) {
            return (detail::to_sequence_field_range(config));
        } else {
            return (detail::to_sequence_address_range(config));
        }
    } else {
        return (detail::to_list_range(config));
    }
}

template <typename Config> auto to_range_cycle(const Config& config)
{
    return (to_range(config) | ranges::views::cycle);
}

using field_list_config = list_config<uint32_t>;
using field_sequence_config = sequence_config<uint32_t>;
using ipv4_list_config = list_config<libpacket::type::ipv4_address>;
using ipv4_sequence_config = sequence_config<libpacket::type::ipv4_address>;
using ipv6_list_config = list_config<libpacket::type::ipv6_address>;
using ipv6_sequence_config = sequence_config<libpacket::type::ipv6_address>;
using mac_list_config = list_config<libpacket::type::mac_address>;
using mac_sequence_config = sequence_config<libpacket::type::mac_address>;

using config = std::variant<field_list_config,
                            field_sequence_config,
                            ipv4_list_config,
                            ipv4_sequence_config,
                            ipv6_list_config,
                            ipv6_sequence_config,
                            mac_list_config,
                            mac_sequence_config>;

using value = std::variant<uint32_t,
                           libpacket::type::ipv4_address,
                           libpacket::type::ipv6_address,
                           libpacket::type::mac_address>;

} // namespace openperf::packet::generator::traffic::modifier

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_MODIFIER_HPP_ */
