#include <functional>
#include <tuple>
#include <vector>

#include "core/op_log.h"
#include "packet/generator/traffic/header/expand.hpp"
#include "packet/generator/traffic/header/container.hpp"
#include "packet/generator/traffic/protocol/custom.hpp"

/*
 * This is basically a cut and paste of expand.tcc, but adapted
 * to the custom protocol type. The custom protocol is sufficiently
 * weird that it's easier to do this than try to make it fit into the
 * existing template based expansion.
 */

namespace openperf::packet::generator::traffic::header {

template <typename T> struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T, size_t... N>
auto to_tuple_impl(const std::vector<T>& items, std::index_sequence<N...>)
{
    return (std::make_tuple(items[N]...));
}

template <size_t N, typename T> auto to_tuple(const std::vector<T>& items)
{
    assert(N <= items.size());
    return (to_tuple_impl(items, std::make_index_sequence<N>{}));
}

template <typename ModifierValue>
void set_value_at_offset(std::vector<uint8_t>& buffer,
                         uint16_t offset,
                         const ModifierValue& value)
{
    std::visit(
        [&](const auto& v) {
            using field_type = remove_cvref_t<decltype(v)>;
            auto* field = reinterpret_cast<std::add_pointer_t<field_type>>(
                buffer.data() + offset);
            if constexpr (std::is_integral_v<field_type>) {
                /*
                 * Since we don't have any mask information, we XOR
                 * the data into place. This allows users to control
                 * which bits will actually get set, since they can
                 * zero out this portion of the custom protocol.
                 */
                *field = *field ^ libpacket::type::endian::bswap(v);
            } else {
                *field = v;
            }
        },
        value);
}

template <typename... ModifierOffset, typename Value1, typename Value2>
void set_values_at_offsets(std::vector<uint8_t>& buffer,
                           const std::tuple<ModifierOffset...>& offsets,
                           std::pair<Value1, Value2>&& values)
{
    static_assert(sizeof...(ModifierOffset) == 2);

    set_value_at_offset(buffer, std::get<0>(offsets), values.first);
    set_value_at_offset(buffer, std::get<1>(offsets), values.second);
}

template <typename... ModifierOffset, typename... ModifierValue, size_t... N>
void set_values_at_offsets_impl(std::vector<uint8_t>& buffer,
                                std::tuple<ModifierOffset...>& offsets,
                                std::tuple<ModifierValue...>&& values,
                                std::index_sequence<N...>)
{
    ((set_value_at_offset(buffer, std::get<N>(offsets), std::get<N>(values))),
     ...);
}

template <typename... ModifierOffset, typename... ModifierValue>
void set_values_at_offsets(std::vector<uint8_t>& buffer,
                           std::tuple<ModifierOffset...>& offsets,
                           std::tuple<ModifierValue...>&& values)
{
    static_assert(sizeof...(ModifierOffset) == sizeof...(ModifierValue));

    set_values_at_offsets_impl(
        buffer,
        offsets,
        std::forward<decltype(values)>(values),
        std::make_index_sequence<sizeof...(ModifierOffset)>{});
}

template <typename Modifier>
container do_expand_single(const protocol::custom& header,
                           const std::pair<uint16_t, Modifier>& pair)
{
    auto range = std::visit(
        [](const auto& config) {
            return (ranges::any_view<modifier::value>(to_range(config)));
        },
        pair.second);

    auto tmp = std::vector<uint8_t>(header.data);
    auto headers = container{};
    for (auto&& value : range) {
        set_value_at_offset(tmp, pair.first, value);
        headers.emplace_back(tmp.data(), tmp.size());
    }

    return (headers);
}

template <typename Header, typename... OffsetModifierPair>
container do_expand_cartesian(const Header& header,
                              std::tuple<OffsetModifierPair...>&& tuple)
{
    /* We need to figure out the length of the combined modifier sequence */
    const auto length_visitor = [](const auto& config) -> size_t {
        return (config.length());
    };

    /* Generate an array of modifier lengths */
    auto lengths = std::apply(
        [&](auto&&... items)
            -> std::array<size_t, sizeof...(OffsetModifierPair)> {
            return {(std::visit(length_visitor, items.second))...};
        },
        tuple);

    /*
     * The cartesian product produces a sequence length equal to the product
     * of all lengths;
     */
    auto seq_length = std::accumulate(
        std::begin(lengths), std::end(lengths), 1UL, std::multiplies<size_t>{});

    /*
     * Now, generate a range containing a tuple for each modifier combination
     * in our sequence.
     */
    auto range_visitor = [](const auto& config) {
        return (ranges::any_view<modifier::value, ranges::category::forward>(
            to_range(config)));
    };

    auto modifier_tuples = std::apply(
        [&](auto&&... items) {
            return (ranges::views::cartesian_product(
                std::visit(range_visitor, items.second)...));
        },
        tuple);

    /*
     * Similarly, generate a tuple of offsets. We need this to
     * actually set the values in our base header.
     */
    auto offsets = std::apply(
        [](auto&&... items) { return (std::forward_as_tuple(items.first...)); },
        tuple);

    /*
     * Use the offset and modifier tuples to generate the full
     * sequence of headers
     */
    auto tmp = std::vector<uint8_t>(header.data);
    auto headers = container{};
    for (auto&& values : modifier_tuples | ranges::views::take(seq_length)) {
        set_values_at_offsets(
            tmp, offsets, std::forward<decltype(values)>(values));
        headers.emplace_back(tmp.data(), tmp.size());
    }

    return (headers);
}

template <typename... OffsetModifierPair>
container do_expand_zip(const protocol::custom& header,
                        std::tuple<OffsetModifierPair...>&& tuple)
{
    /* We need to figure out the length of the combined modifier sequence */
    const auto length_visitor = [](const auto& config) -> size_t {
        return (config.length());
    };

    /* Generate an array of modifier lengths */
    auto lengths = std::apply(
        [&](auto&&... items)
            -> std::array<size_t, sizeof...(OffsetModifierPair)> {
            return {(std::visit(length_visitor, items.second))...};
        },
        tuple);

    /*
     * When zipping, the combined length is the lowest common multiple of
     * all the lengths.
     */
    auto seq_length = std::accumulate(
        std::begin(lengths), std::end(lengths), 1UL, std::lcm<size_t, size_t>);

    /*
     * Now, generate a range containing a tuple for each modifier combination
     * in our sequence.
     */
    auto range_visitor = [](const auto& config) {
        return (ranges::any_view<modifier::value>(to_range_cycle(config)));
    };

    auto modifier_tuples = std::apply(
        [&](auto&&... items) {
            return (
                ranges::views::zip(std::visit(range_visitor, items.second)...));
        },
        tuple);

    /*
     * Similarly, generate a tuple of offsets. We need this to
     * actually set the values in our base header.
     */
    auto offsets = std::apply(
        [](auto&&... items) { return (std::forward_as_tuple(items.first...)); },
        tuple);

    /*
     * Use the offset and modifier tuples to generate the full
     * sequence of headers
     */
    auto tmp = std::vector<uint8_t>(header.data);
    auto headers = container{};
    for (auto&& values : modifier_tuples | ranges::views::take(seq_length)) {
        set_values_at_offsets(
            tmp, offsets, std::forward<decltype(values)>(values));
        headers.emplace_back(tmp.data(), tmp.size());
    }

    return (headers);
}

container expand_single(const custom_config& config)
{
    return (do_expand_single(config.header, config.modifiers.front()));
}

template <size_t N> container expand_zip(const custom_config& config)
{
    assert(config.modifiers.size() == N);
    return (do_expand_zip(config.header, to_tuple<N>(config.modifiers)));
}

template <size_t N> container expand_cartesian(const custom_config& config)
{
    assert(config.modifiers.size() == N);
    return (do_expand_cartesian(config.header, to_tuple<N>(config.modifiers)));
}

container expand(const custom_config& config)
{
    const auto nb_modifiers = config.modifiers.size();

    assert(nb_modifiers <= 1 || config.mux != modifier_mux::none);

    switch (nb_modifiers) {
    case 8:
        return (config.mux == modifier_mux::zip ? expand_zip<8>(config)
                                                : expand_cartesian<8>(config));
    case 7:
        return (config.mux == modifier_mux::zip ? expand_zip<7>(config)
                                                : expand_cartesian<7>(config));
    case 6:
        return (config.mux == modifier_mux::zip ? expand_zip<6>(config)
                                                : expand_cartesian<6>(config));
    case 5:
        return (config.mux == modifier_mux::zip ? expand_zip<5>(config)
                                                : expand_cartesian<5>(config));
    case 4:
        return (config.mux == modifier_mux::zip ? expand_zip<4>(config)
                                                : expand_cartesian<4>(config));
    case 3:
        return (config.mux == modifier_mux::zip ? expand_zip<3>(config)
                                                : expand_cartesian<3>(config));
    case 2:
        return (config.mux == modifier_mux::zip ? expand_zip<2>(config)
                                                : expand_cartesian<2>(config));
    case 1:
        return (expand_single(config));
    default:
        /* Should have no modifiers... */
        if (nb_modifiers) {
            /*
             * Since we don't have the functions to expand this header, we just
             * return the base.  But at least we can whine about it!
             */
            OP_LOG(OP_LOG_ERROR,
                   "Cannot expand custom header; too many modifiers!\n");
        }
        auto c = container{};
        c.emplace_back(config.header.data.data(), config.header.data.size());
        return (c);
    }
}

} // namespace openperf::packet::generator::traffic::header
