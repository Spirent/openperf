#ifndef _OP_CPU_ISPC_FUNCTION_WRAPPER_HPP_
#define _OP_CPU_ISPC_FUNCTION_WRAPPER_HPP_

#include "ispc/ispc.hpp"
#include "instruction_set.hpp"

namespace openperf::cpu::internal {

using openperf::cpu::instruction_set;

template <typename Function> class function_wrapper
{
public:
    using functions_map = std::array<std::pair<instruction_set, Function>,
                                     static_cast<int>(instruction_set::MAX)>;

    static constexpr functions_map functions;

public:
    template <instruction_set S, typename... Args>
    auto operator()(Args&&... args)
    {
        return std::invoke(std::forward<Function>(function(S)),
                           std::forward<Args>(args)...);
    }

    constexpr Function function(instruction_set set)
    {
        auto function_it =
            std::find_if(std::begin(functions),
                         std::end(functions),
                         [set](auto& item) { return item.first == set; });

        return (function_it == std::end(functions)) ? nullptr
                                                    : function_it->second;
    }

    constexpr instruction_set instruction_set(Function f)
    {
        auto function_it =
            std::find_if(std::begin(functions),
                         std::end(functions),
                         [f](auto& item) { return item.second == f; });

        return (function_it == std::end(functions)) ? instruction_set::NONE
                                                    : function_it->first;
    }
};

#define ISPC_FUNCTION_WRAPPER_INIT_FUNCTION_DATA(f)                            \
    {                                                                          \
        {instruction_set::SCALAR, scalar::f},                                  \
            {instruction_set::AUTO,                                            \
             (ispc::automatic_enabled ? simd::f : nullptr)},                   \
            {instruction_set::SSE2,                                            \
             (ispc::sse2_enabled ? simd::f##_sse2 : nullptr)},                 \
            {instruction_set::SSE4,                                            \
             (ispc::sse4_enabled ? simd::f##_sse4 : nullptr)},                 \
            {instruction_set::AVX,                                             \
             (ispc::avx_enabled ? simd::f##_avx : nullptr)},                   \
            {instruction_set::AVX2,                                            \
             (ispc::avx2_enabled ? simd::f##_avx2 : nullptr)},                 \
            {instruction_set::AVX512,                                          \
             (ispc::avx512skx_enabled ? simd::f##_avx512skx : nullptr)},       \
    }

#define ISPC_FUNCTION_WRAPPER_INIT(ret, f, ...)                                \
    namespace scalar {                                                         \
    extern ret f(__VA_ARGS__);                                                 \
    }                                                                          \
    namespace simd {                                                           \
    extern "C" ret f(__VA_ARGS__);                                             \
    extern "C" ret f##_sse2(__VA_ARGS__);                                      \
    extern "C" ret f##_sse4(__VA_ARGS__);                                      \
    extern "C" ret f##_avx(__VA_ARGS__);                                       \
    extern "C" ret f##_avx2(__VA_ARGS__);                                      \
    extern "C" ret f##_avx512skx(__VA_ARGS__);                                 \
    }                                                                          \
    template <>                                                                \
    constexpr function_wrapper<decltype(&simd::f)>::functions_map              \
        function_wrapper<decltype(&simd::f)>::functions = {                    \
            ISPC_FUNCTION_WRAPPER_INIT_FUNCTION_DATA(f)}

} // namespace openperf::cpu::internal

#endif // _OP_CPU_ISPC_FUNCTION_WRAPPER_HPP_
