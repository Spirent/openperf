#ifndef _LIB_REGURGITATE_FUNCTIONS_HPP_
#define _LIB_REGURGITATE_FUNCTIONS_HPP_

#include "function_wrapper.hpp"

#define ISPC_MERGE_AND_SORT(key_type, val_type)                                \
    using merge_##key_type##_##val_type##_fn =                                 \
        unsigned (*)(key_type in_means[],                                      \
                     val_type in_weights[],                                    \
                     unsigned length,                                          \
                     key_type out_means[],                                     \
                     val_type out_weights[],                                   \
                     unsigned out_lenght);                                     \
    ISPC_FUNCTION_WRAPPER_INIT(unsigned,                                       \
                               merge_##key_type##_##val_type,                  \
                               key_type[],                                     \
                               val_type[],                                     \
                               unsigned,                                       \
                               key_type[],                                     \
                               val_type[],                                     \
                               unsigned);                                      \
    using sort_##key_type##_##val_type##_fn = void (*)(key_type means[],       \
                                                       val_type weights[],     \
                                                       unsigned length,        \
                                                       key_type scratch_m[],   \
                                                       val_type scratch_w[]);  \
    ISPC_FUNCTION_WRAPPER_INIT(void,                                           \
                               sort_##key_type##_##val_type,                   \
                               key_type[],                                     \
                               val_type[],                                     \
                               unsigned,                                       \
                               key_type[],                                     \
                               val_type[]);

ISPC_MERGE_AND_SORT(float, double);
ISPC_MERGE_AND_SORT(float, float);

#undef ISPC_MERGE_AND_SORT

namespace regurgitate::impl {

template <typename T> class singleton
{
public:
    static T& instance()
    {
        static T instance;
        return (instance);
    }

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton) = delete;

protected:
    singleton(){};
};

struct functions : singleton<functions>
{
    functions();

    function_wrapper<merge_float_double_fn> merge_float_double_impl = {
        "merge<float, double>", nullptr};
    function_wrapper<merge_float_float_fn> merge_float_float_impl = {
        "merge<float, float>", nullptr};

    function_wrapper<sort_float_double_fn> sort_float_double_impl = {
        "sort<float, double>", nullptr};
    function_wrapper<sort_float_float_fn> sort_float_float_impl = {
        "sort<float, float>", nullptr};
};

} // namespace regurgitate::impl

#endif /* _LIB_REGURGITATE_FUNCTIONS_HPP */
