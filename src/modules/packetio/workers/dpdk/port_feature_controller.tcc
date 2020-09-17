#include "packetio/workers/dpdk/port_feature_controller.hpp"

namespace openperf::packetio::dpdk {

namespace detail {

template <typename Tuple, typename T, std::size_t index = 0>
constexpr size_t tuple_index()
{
    if constexpr (std::is_same_v<std::tuple_element_t<index, Tuple>,
                                 std::unique_ptr<T>>) {
        return (index);
    } else {
        return (tuple_index<Tuple, T, index + 1>());
    }
}

template <typename Tuple, typename Function, typename Container, size_t... I>
constexpr void for_each_type_at_index_impl(Function&& f,
                                           Container&& c,
                                           size_t idx,
                                           std::index_sequence<I...>)
{
    (f(c.template get<std::tuple_element_t<I, Tuple>>(idx)), ...);
}

template <typename Tuple, typename Function, typename Container>
constexpr void for_each_type_at_index(Function&& f, Container&& c, size_t idx)
{
    for_each_type_at_index_impl<Tuple>(
        std::forward<Function>(f),
        std::forward<Container>(c),
        idx,
        std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

} // namespace detail

template <typename... Features>
port_feature_controller<Features...>::port_feature_controller()
{
    const auto port_indexes = topology::get_ports();

    std::transform(
        std::begin(port_indexes),
        std::end(port_indexes),
        std::back_inserter(m_features),
        [](const auto& idx) {
            return (std::make_tuple((std::make_unique<Features>(idx))...));
        });
}

template <typename... Features>
port_feature_controller<Features...>::port_feature_controller(
    port_feature_controller&& other) noexcept
    : m_features(std::move(other.m_features))
{}

template <typename... Features>
port_feature_controller<Features...>&
port_feature_controller<Features...>::operator=(
    port_feature_controller&& other) noexcept
{
    if (this != &other) { m_features = std::move(other.m_features); }
    return (*this);
}

template <typename... Features>
template <typename T>
T& port_feature_controller<Features...>::get(size_t idx)
{
    return (const_cast<T&>(
        const_cast<const port_feature_controller*>(this)->get<T>(idx)));
}

template <typename... Features>
template <typename T>
const T& port_feature_controller<Features...>::get(size_t idx) const
{
    const auto tuple_idx = detail::tuple_index<container_tuple, T>();
    const auto& features = m_features.template get<tuple_idx>();
    const auto cursor = std::find_if(
        std::begin(features), std::end(features), [&](const auto& item) {
            return (item->port_id() == idx);
        });

    if (cursor == std::end(features)) {
        throw std::out_of_range("Port idx " + std::to_string(idx)
                                + " not found");
    }

    return (*(cursor->get()));
}

template <typename... Features>
template <typename T>
const std::vector<std::unique_ptr<T>>&
port_feature_controller<Features...>::get() const
{
    const auto tuple_idx = detail::tuple_index<container_tuple, T>();
    return (m_features.template get<tuple_idx>());
}

template <typename Feature>
void maybe_toggle_sink_feature(const worker::fib& fib,
                               size_t port_idx,
                               Feature&& feature)
{
    if (need_sink_feature(fib, port_idx, feature)) {
        feature.enable();
    } else {
        feature.disable();
    }
}

template <typename... Features>
void sink_feature_controller<Features...>::update(const worker::fib& fib,
                                                  size_t port_idx)
{
    using feature_tuple = std::tuple<Features...>;

    detail::for_each_type_at_index<feature_tuple>(
        [&](auto&& feature) {
            maybe_toggle_sink_feature(
                fib, port_idx, std::forward<decltype(feature)>(feature));
        },
        *this,
        port_idx);
}

template <typename Feature>
void maybe_toggle_source_feature(const worker::tib& tib,
                                 size_t port_idx,
                                 Feature&& feature)
{
    if (need_source_feature(tib, port_idx, feature)) {
        feature.enable();
    } else {
        feature.disable();
    }
}

template <typename... Features>
void source_feature_controller<Features...>::update(const worker::tib& tib,
                                                    size_t port_idx)
{
    using feature_tuple = std::tuple<Features...>;

    detail::for_each_type_at_index<feature_tuple>(
        [&](auto&& feature) {
            maybe_toggle_source_feature(
                tib, port_idx, std::forward<decltype(feature)>(feature));
        },
        *this,
        port_idx);
}

} // namespace openperf::packetio::dpdk
