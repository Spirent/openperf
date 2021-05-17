#ifndef _LIB_REGURGITATE_TEST_API_HPP_
#define _LIB_REGURGITATE_TEST_API_HPP_

#include <random>

#include "regurgitate/functions.hpp"
#include "regurgitate/instruction_set.hpp"

namespace regurgitate::test {

#define ISPC_GET_FUNCTION_WRAPPER(key_type, val_type)                          \
    template <typename K, typename V>                                          \
    std::enable_if_t<                                                          \
        std::conjunction_v<std::is_same<K, key_type>,                          \
                           std::is_same<V, val_type>>,                         \
        regurgitate::function_wrapper<sort_##key_type##_##val_type##_fn>&>     \
    get_sort_wrapper()                                                         \
    {                                                                          \
        auto& functions = regurgitate::functions::instance();                  \
        return (functions.sort_##key_type##_##val_type##_impl);                \
    }                                                                          \
    template <typename K, typename V>                                          \
    std::enable_if_t<                                                          \
        std::conjunction_v<std::is_same<K, key_type>,                          \
                           std::is_same<V, val_type>>,                         \
        regurgitate::function_wrapper<merge_##key_type##_##val_type##_fn>&>    \
    get_merge_wrapper()                                                        \
    {                                                                          \
        auto& functions = regurgitate::functions::instance();                  \
        return (functions.merge_##key_type##_##val_type##_impl);               \
    }

ISPC_GET_FUNCTION_WRAPPER(float, double)
ISPC_GET_FUNCTION_WRAPPER(float, float)

#undef ISPC_GET_FUNCTION_WRAPPER

template <typename Container> void fill_random(Container& c, size_t size)
{
    auto device = std::random_device{};
    auto generator = std::mt19937(device());
    auto dist = std::uniform_real_distribution<>(0.0, 1000000.0);

    for (auto i = 0U; i < size; i++) { c.push_back(dist(generator)); }
}

using instruction_sets = std::vector<instruction_set::type>;

instruction_sets get_instruction_sets();

} // namespace regurgitate::test

#endif /* _LIB_REGURGITATE_TEST_API_HPP_ */
