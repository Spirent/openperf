#ifndef _OP_CPU_ISPC_FUNCTION_WRAPPER_HPP_
#define _OP_CPU_ISPC_FUNCTION_WRAPPER_HPP_

#include <functional>

#include "cpu/instruction_set.hpp"

namespace openperf::cpu::internal {

template <typename Function> class function_wrapper
{
public:
    using functions_map =
        std::array<std::pair<instruction_set::type, Function>,
                   static_cast<int>(instruction_set::type::MAX)>;

    static constexpr functions_map functions;

    template <instruction_set::type S, typename... Args>
    auto operator()(Args&&... args)
    {
        return std::invoke(std::forward<Function>(function(S)),
                           std::forward<Args>(args)...);
    }

    constexpr Function function(instruction_set::type set)
    {
        auto function_it =
            std::find_if(std::begin(functions),
                         std::end(functions),
                         [set](auto& item) { return item.first == set; });

        return (function_it == std::end(functions)) ? nullptr
                                                    : function_it->second;
    }

    constexpr instruction_set::type instruction_set(Function f)
    {
        auto function_it =
            std::find_if(std::begin(functions),
                         std::end(functions),
                         [f](auto& item) { return item.second == f; });

        return (function_it == std::end(functions))
                   ? instruction_set::type::NONE
                   : function_it->first;
    }
};

} // namespace openperf::cpu::internal

namespace instruction_set = openperf::cpu::instruction_set;

#define ISPC_FUNCTION_WRAPPER_INIT_FUNCTION_DATA(f)                            \
    {                                                                          \
        {instruction_set::type::SCALAR, scalar::f},                            \
            {instruction_set::type::AUTO,                                      \
             (instruction_set::automatic_enabled ? ispc::f : nullptr)},        \
            {instruction_set::type::SSE2,                                      \
             (instruction_set::sse2_enabled ? ispc::f##_sse2 : nullptr)},      \
            {instruction_set::type::SSE4,                                      \
             (instruction_set::sse4_enabled ? ispc::f##_sse4 : nullptr)},      \
            {instruction_set::type::AVX,                                       \
             (instruction_set::avx_enabled ? ispc::f##_avx : nullptr)},        \
            {instruction_set::type::AVX2,                                      \
             (instruction_set::avx2_enabled ? ispc::f##_avx2 : nullptr)},      \
        {                                                                      \
            instruction_set::type::AVX512SKX,                                  \
                (instruction_set::avx512skx_enabled ? ispc::f##_avx512skx      \
                                                    : nullptr)                 \
        }                                                                      \
    }

#define ISPC_FUNCTION_WRAPPER_INIT(ret, f, ...)                                \
    namespace scalar {                                                         \
    extern ret f(__VA_ARGS__);                                                 \
    }                                                                          \
    namespace ispc {                                                           \
    extern "C" ret f(__VA_ARGS__);                                             \
    extern "C" ret f##_sse2(__VA_ARGS__);                                      \
    extern "C" ret f##_sse4(__VA_ARGS__);                                      \
    extern "C" ret f##_avx(__VA_ARGS__);                                       \
    extern "C" ret f##_avx2(__VA_ARGS__);                                      \
    extern "C" ret f##_avx512skx(__VA_ARGS__);                                 \
    }                                                                          \
    template <>                                                                \
    constexpr openperf::cpu::internal::function_wrapper<decltype(&ispc::f)>::  \
        functions_map openperf::cpu::internal::function_wrapper<decltype(      \
            &ispc::f)>::functions = {                                          \
            ISPC_FUNCTION_WRAPPER_INIT_FUNCTION_DATA(f)}

#endif // _OP_CPU_ISPC_FUNCTION_WRAPPER_HPP_
