#ifndef _OP_CPU_LOAD_TYPES_HPP_
#define _OP_CPU_LOAD_TYPES_HPP_

#include <utility>

namespace openperf::cpu {

/**
 * A list of types
 **/
template <typename... Types> struct type_list
{};

namespace detail {

/**
 * Concatenate two type lists
 **/
template <typename... Types> struct concat;
template <typename... Left, typename... Right>
struct concat<type_list<Left...>, type_list<Right...>>
{
    using type = type_list<Left..., Right...>;
};

/**
 * Generate the cartesian product of two type lists
 **/
template <typename Left, typename Right> struct cartesian_product;

/* The product of anything with the empty set is... the empty set. */
template <typename... Types>
struct cartesian_product<type_list<>, type_list<Types...>>
{
    using type = type_list<>;
};

template <typename T, typename... Ts, typename... Us>
struct cartesian_product<type_list<T, Ts...>, type_list<Us...>>
{
    /*
     * Recursively construct pairs of types from the type list.
     * The result is a type list of pairs representing the cartesian
     * product of types in the two lists.
     */
    using type = typename concat<
        type_list<std::pair<T, Us>...>,
        typename cartesian_product<type_list<Ts...>,
                                   type_list<Us...>>::type>::type;
};

} // namespace detail

namespace instruction_set {
struct scalar
{};
struct sse2
{};
struct sse4
{};
struct avx
{};
struct avx2
{};
struct avx512
{};
struct neon
{};
}; // namespace instruction_set

using instruction_sets = type_list<instruction_set::scalar,
                                   instruction_set::sse2,
                                   instruction_set::sse4,
                                   instruction_set::avx,
                                   instruction_set::avx2,
                                   instruction_set::avx512,
                                   instruction_set::neon>;

using data_types = type_list<int32_t, int64_t, float, double>;

using load_types =
    typename detail::cartesian_product<instruction_sets, data_types>::type;

} // namespace openperf::cpu

#endif /* _OP_CPU_LOAD_TYPES_HPP_ */
