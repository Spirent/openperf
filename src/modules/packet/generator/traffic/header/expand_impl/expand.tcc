#ifndef _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPAND_TCC_
#define _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPAND_TCC_

#include <functional>
#include <vector>
#include <tuple>

#include "core/op_log.h"
#include "packet/generator/traffic/header/config.hpp"
#include "packet/generator/traffic/header/container.hpp"

namespace openperf::packet::generator::traffic::header::detail {

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

template <typename Header, typename ModifierValue>
void set_header_field(Header& header,
                      typename Header::field_name field,
                      const ModifierValue& value)
{
    const auto set_visitor = [&](const auto& v) { header.set_field(field, v); };
    std::visit(set_visitor, value);
}

/**
 * Annoying, the ranges zip/cartesian functions return a pair like value
 * when the number of ranges == 2, so we need an explicit overload to
 * handle that case.
 */
template <typename Header,
          typename... ModifierField,
          typename Value1,
          typename Value2>
void set_header_fields(Header& header,
                       const std::tuple<ModifierField...>& fields,
                       std::pair<Value1, Value2>&& values)
{
    static_assert(sizeof...(ModifierField) == 2);

    set_header_field(header, std::get<0>(fields), values.first);
    set_header_field(header, std::get<1>(fields), values.second);
}

template <typename Header,
          typename... ModifierField,
          typename... ModifierValue,
          size_t... N>
void set_header_fields_impl(Header& header,
                            std::tuple<ModifierField...>& fields,
                            std::tuple<ModifierValue...>&& values,
                            std::index_sequence<N...>)
{
    ((set_header_field(header, std::get<N>(fields), std::get<N>(values))), ...);
}

template <typename Header, typename... ModifierField, typename... ModifierValue>
void set_header_fields(Header& header,
                       std::tuple<ModifierField...>& fields,
                       std::tuple<ModifierValue...>&& values)
{
    static_assert(sizeof...(ModifierField) == sizeof...(ModifierValue));

    set_header_fields_impl(
        header,
        fields,
        std::forward<decltype(values)>(values),
        std::make_index_sequence<sizeof...(ModifierField)>{});
}

template <typename Header, typename Field, typename Modifier>
container do_expand_single(const Header& header,
                           const std::pair<Field, Modifier>& pair)
{
    auto range_visitor = [](const auto& config) {
        return (ranges::any_view<modifier::value>(to_range(config)));
    };

    auto range = std::visit(range_visitor, pair.second);

    auto tmp = Header{header};
    auto headers = container{};
    for (auto&& value : range) {
        set_header_field(tmp, pair.first, value);
        headers.emplace_back(std::addressof(tmp), Header::protocol_length);
    }

    return (headers);
}

template <typename Header, typename... FieldModifierPair>
container do_expand_cartesian(const Header& header,
                              std::tuple<FieldModifierPair...>&& tuple)
{
    /* We need to figure out the length of the combined modifier sequence */
    const auto length_visitor = [](const auto& config) -> size_t {
        return (config.length());
    };

    /* Generate an array of modifier lengths */
    auto lengths = std::apply(
        [&](auto&&... items)
            -> std::array<size_t, sizeof...(FieldModifierPair)> {
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
            to_range_cycle(config)));
    };

    auto modifier_tuples = std::apply(
        [&](auto&&... items) {
            return (ranges::views::cartesian_product(
                std::visit(range_visitor, items.second)...));
        },
        tuple);

    /*
     * Similarly, generate a tuple of field identifiers. We need this to
     * actually set the values in our base header.
     */
    auto fields = std::apply(
        [](auto&&... items) { return (std::forward_as_tuple(items.first...)); },
        tuple);

    /*
     * Use the field and modifier tuples to generate the full sequence of
     * headers
     */
    auto tmp = Header{header};
    auto headers = container{};
    for (auto&& values : modifier_tuples | ranges::views::take(seq_length)) {
        set_header_fields(tmp, fields, std::forward<decltype(values)>(values));
        headers.emplace_back(std::addressof(tmp), Header::protocol_length);
    }

    return (headers);
}

template <typename Header, typename... FieldModifierPair>
container do_expand_zip(const Header& header,
                        std::tuple<FieldModifierPair...>&& tuple)
{
    /* We need to figure out the length of the combined modifier sequence */
    const auto length_visitor = [](const auto& config) -> size_t {
        return (config.length());
    };

    /* Generate an array of modifier lengths */
    auto lengths = std::apply(
        [&](auto&&... items)
            -> std::array<size_t, sizeof...(FieldModifierPair)> {
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
     * Similarly, generate a tuple of field identifiers. We need this to
     * actually set the values in our base header.
     */
    auto fields = std::apply(
        [](auto&&... items) { return (std::forward_as_tuple(items.first...)); },
        tuple);

    /*
     * Use the field and modifier tuples to generate the full sequence of
     * headers
     */
    auto tmp = Header{header};
    auto headers = container{};
    for (auto&& values : modifier_tuples | ranges::views::take(seq_length)) {
        set_header_fields(tmp, fields, std::forward<decltype(values)>(values));
        headers.emplace_back(std::addressof(tmp), Header::protocol_length);
    }

    return (headers);
}

template <typename Header> container expand_single(const config<Header>& config)
{
    return (do_expand_single(config.header, config.modifiers.front()));
}

template <size_t N, typename Header>
container expand_cartesian(const config<Header>& config)
{
    static_assert(N <= Header::protocol_field_count,
                  "Too many fields to multiply");
    assert(config.modifiers.size() == N);
    return (do_expand_cartesian(config.header, to_tuple<N>(config.modifiers)));
}

template <size_t N, typename Header>
container expand_zip(const config<Header>& config)
{
    static_assert(N <= Header::protocol_field_count, "Too many fields to zip");
    assert(config.modifiers.size() == N);
    return (do_expand_zip(config.header, to_tuple<N>(config.modifiers)));
}

template <typename Header> container expand(const config<Header>& config)
{
    const auto nb_modifiers = config.modifiers.size();

    assert(nb_modifiers == 1 || config.mux != modifier_mux::none);
    assert(nb_modifiers <= Header::protocol_field_count);

    /*
     * Use constexpr and the protocol's field count to skip instantiating
     * template functions that we can't use.
     */
    switch (nb_modifiers) {
    case 8:
        if constexpr (8 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<8>(config)
                        : expand_cartesian<8>(config));
        } else {
            [[fallthrough]];
        }
    case 7:
        if constexpr (7 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<7>(config)
                        : expand_cartesian<7>(config));
        } else {
            [[fallthrough]];
        }
    case 6:
        if constexpr (6 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<6>(config)
                        : expand_cartesian<6>(config));
        } else {
            [[fallthrough]];
        }
    case 5:
        if constexpr (5 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<5>(config)
                        : expand_cartesian<5>(config));
        } else {
            [[fallthrough]];
        }
    case 4:
        if constexpr (4 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<4>(config)
                        : expand_cartesian<4>(config));
        } else {
            [[fallthrough]];
        }
    case 3:
        if constexpr (3 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<3>(config)
                        : expand_cartesian<3>(config));
        } else {
            [[fallthrough]];
        }
    case 2:
        if constexpr (2 <= Header::protocol_field_count) {
            return (config.mux == modifier_mux::zip
                        ? expand_zip<2>(config)
                        : expand_cartesian<2>(config));
        } else {
            [[fallthrough]];
        }
    case 1:
        return (expand_single(config));
    default: {
        /* Hopefully we have no modifiers; else we have too many. */
        if (nb_modifiers) {
            /*
             * Since we don't have the functions to expand this header, we just
             * return the base.  But at least we can whine about it!
             */
            OP_LOG(OP_LOG_ERROR, "Cannot expand header; too many modifiers!\n");
        }
        auto c = container{};
        c.emplace_back(std::addressof(config.header), Header::protocol_length);
        return (c);
    }
    }
}

} // namespace openperf::packet::generator::traffic::header::detail

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPAND_TCC_ */
