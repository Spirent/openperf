#ifndef _OP_CPU_TARGET_ISPC_HPP_
#define _OP_CPU_TARGET_ISPC_HPP_

#include <vector>
#include "target.hpp"

namespace openperf::cpu::internal {

#define ISPC_FUNCTION(f, ispc_type, type)                                      \
    namespace ispc {                                                           \
    extern "C" {                                                               \
    uint64_t f##_##ispc_type(const type*, const type*, uint16_t);              \
    uint64_t f##_##ispc_type##_sse2(const type*, const type*, uint16_t);       \
    uint64_t f##_##ispc_type##_sse4(const type*, const type*, uint16_t);       \
    uint64_t f##_##ispc_type##_avx(const type*, const type*, uint16_t);        \
    uint64_t f##_##ispc_type##_avx2(const type*, const type*, uint16_t);       \
    uint64_t f##_##ispc_type##_avx512skx(const type*, const type*, uint16_t);  \
    }                                                                          \
    }

ISPC_FUNCTION(multiplicate_matrix, uint32, uint32_t)
ISPC_FUNCTION(multiplicate_matrix, uint64, uint64_t)
ISPC_FUNCTION(multiplicate_matrix, double, double)
ISPC_FUNCTION(multiplicate_matrix, float, float)

#define ISPC_FUNCTION_TUPLE(f)                                                 \
    {instruction_set::SSE2,                                                    \
     enabled(instruction_set::SSE2) ? f##_sse2 : nullptr},                     \
        {instruction_set::SSE4,                                                \
         enabled(instruction_set::SSE4) ? f##_sse4 : nullptr},                 \
        {instruction_set::AVX,                                                 \
         enabled(instruction_set::AVX) ? f##_avx : nullptr},                   \
        {instruction_set::AVX2,                                                \
         enabled(instruction_set::AVX2) ? f##_avx2 : nullptr},                 \
    {                                                                          \
        instruction_set::AVX512SKX,                                            \
            enabled(instruction_set::AVX512SKX) ? f##_avx512skx : nullptr      \
    }

template <class T> class target_ispc : public target
{
private:
    using function = uint64_t (*)(const T*, const T*, uint16_t);
    using function_array = std::array<std::pair<instruction_set, function>,
                                      static_cast<int>(instruction_set::MAX)>;

    static constexpr function_array functions = []() constexpr->function_array
    {
        if constexpr (std::is_same<T, uint32_t>::value)
            return {{ISPC_FUNCTION_TUPLE(ispc::multiplicate_matrix_uint32)}};
        else if constexpr (std::is_same<T, uint64_t>::value)
            return {{ISPC_FUNCTION_TUPLE(ispc::multiplicate_matrix_uint64)}};
        else if constexpr (std::is_same<T, double>::value)
            return {{ISPC_FUNCTION_TUPLE(ispc::multiplicate_matrix_double)}};
        else if constexpr (std::is_same<T, float>::value)
            return {{ISPC_FUNCTION_TUPLE(ispc::multiplicate_matrix_float)}};
        return {{}};
    }
    ();

private:
    constexpr static size_t size = 32;

    std::vector<T> matrix_a;
    std::vector<T> matrix_b;
    std::vector<T> matrix_r;
    function m_operation;

public:
    target_ispc(cpu::instruction_set iset)
    {
        matrix_a.resize(size * size);
        matrix_b.resize(size * size);

        for (size_t i = 0; i < size * size; ++i) {
            auto row = i / size;
            auto col = i % size;
            matrix_a[i] = row * col;
            matrix_b[i] = static_cast<T>(size * size) / (row * col + 1);
        }

        for (auto& [set, function] : functions) {
            if (set == iset) {
                m_operation = function;
                break;
            }
        }

        if (m_operation == nullptr)
            throw std::runtime_error("Instruction set not supported");
    }

    ~target_ispc() override = default;

    [[clang::optnone]] uint64_t operation() const override
    {
        return m_operation(matrix_a.data(), matrix_b.data(), size);
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_ISPC_HPP_
