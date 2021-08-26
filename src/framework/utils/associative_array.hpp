#ifndef _OP_UTILS_ASSOCIATIVE_ARRAY_HPP_
#define _OP_UTILS_ASSOCIATIVE_ARRAY_HPP_

#include <array>
#include <optional>
#include <utility>

namespace openperf::utils {

template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

template <typename AssociativeArray,
          typename Key = decltype(
              std::declval<typename AssociativeArray::value_type>().first),
          typename Value = decltype(
              std::declval<typename AssociativeArray::value_type>().second)>
constexpr std::optional<Value> key_to_value(const AssociativeArray& array,
                                            const Key& key)
{
    auto cursor = std::begin(array), end = std::end(array);
    while (cursor != end) {
        if (cursor->first == key) { return (cursor->second); }
        cursor++;
    }

    return (std::nullopt);
}

template <typename AssociativeArray,
          typename Key = decltype(
              std::declval<typename AssociativeArray::value_type>().first),
          typename Value = decltype(
              std::declval<typename AssociativeArray::value_type>().second)>
constexpr std::optional<Key> value_to_key(const AssociativeArray& array,
                                          const Value& value)
{
    auto cursor = std::begin(array), end = std::end(array);
    while (cursor != end) {
        if (cursor->second == value) { return (cursor->first); }
        cursor++;
    }

    return (std::nullopt);
}

} // namespace openperf::utils

#endif /* _OP_UTILS_ASSOCIATIVE_ARRAY_HPP_ */
