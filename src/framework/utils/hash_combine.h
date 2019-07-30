#ifndef _ICP_UTILS_HASH_COMBINE_H_
#define _ICP_UTILS_HASH_COMBINE_H_

#include <type_traits>
#include <utility>

namespace icp::utils {

/*
 * This hash combining function is shamelessly taken from the boost hash_combine
 * implementation.
 */
template <typename T>
size_t hash_combine(size_t seed, const T& value)
{
    std::hash<T> hasher;
    return (seed ^ hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

/**
 * We recursively iterate over the tuple types to feed the values into
 * the combining function.  The std::enable_if_t parameter determines
 * whether we terminate or hash and continue.
 **/
template <size_t N = 0, typename... Types>
std::enable_if_t<N == sizeof...(Types), size_t>
hash_combine_tuple(size_t seed, const std::tuple<Types...>&)
{
    return (seed);
}

template <size_t N, typename... Types>
std::enable_if_t<N < sizeof...(Types), size_t>
hash_combine_tuple(size_t seed, const std::tuple<Types...>& tuple)
{
    return (hash_combine_tuple<N + 1>(hash_combine(seed, std::get<N>(tuple)), tuple));
}

}

/* Add pair/tuple specializations to std::hash */
namespace std {

template <typename... Types>
struct hash<std::tuple<Types...>>
{
    size_t operator()(const std::tuple<Types...>& tuple) const
    {
        return (icp::utils::hash_combine_tuple<0>(size_t{0}, tuple));
    }
};

template <typename First, typename Second>
struct hash<std::pair<First, Second>>
{
    size_t operator()(const std::pair<First, Second>& pair) const
    {
        return (icp::utils::hash_combine(
                    icp::utils::hash_combine(size_t{0}, pair.first),
                    pair.second));
    }
};

}

#endif /* _ICP_UTILS_HASH_COMBINE_H_ */
