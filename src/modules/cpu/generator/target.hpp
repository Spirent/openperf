#ifndef _OP_CPU_GENERATOR_TARGET_HPP_
#define _OP_CPU_GENERATOR_TARGET_HPP_

#include <numeric>
#include <variant>
#include <vector>

#include "cpu/generator/load_types.hpp"
#include "cpu/generator/matrix_functions.hpp"

namespace openperf::cpu::generator {

template <typename InstructionSet, typename Wrapper, typename DataType>
unsigned do_matrix_ops(const Wrapper& wrapper,
                       const DataType a[],
                       const DataType b[],
                       DataType c[],
                       size_t length)
{
    if constexpr (std::is_same_v<InstructionSet, instruction_set::scalar>) {
        auto fn = wrapper.function(instruction_set::type::SCALAR);
        fn(a, b, c, length);
    } else if constexpr (std::is_same_v<InstructionSet,
                                        instruction_set::sse2>) {
        auto fn = wrapper.function(instruction_set::type::SSE2);
        fn(a, b, c, length);
    } else if constexpr (std::is_same_v<InstructionSet,
                                        instruction_set::sse4>) {
        auto fn = wrapper.function(instruction_set::type::SSE4);
        fn(a, b, c, length);
    } else if constexpr (std::is_same_v<InstructionSet, instruction_set::avx>) {
        auto fn = wrapper.function(instruction_set::type::AVX);
        fn(a, b, c, length);
    } else if constexpr (std::is_same_v<InstructionSet,
                                        instruction_set::avx2>) {
        auto fn = wrapper.function(instruction_set::type::AVX2);
        fn(a, b, c, length);
    } else if constexpr (std::is_same_v<InstructionSet,
                                        instruction_set::avx512>) {
        auto fn = wrapper.function(instruction_set::type::AVX512SKX);
        fn(a, b, c, length);
    }

    return (1);
}

template <typename InstructionSet, typename DataType> class target_op_impl
{
    static constexpr auto size = 32;

    std::vector<DataType> a;
    std::vector<DataType> b;
    mutable std::vector<DataType> r;

public:
    target_op_impl()
        : a(size * size)
        , b(size * size)
        , r(size * size, 0)
    {
        std::iota(std::begin(a), std::end(a), 1);
        std::iota(std::begin(b), std::end(b), 1);
    }

    unsigned operator()() const
    {
        if constexpr (std::is_same_v<DataType, double>) {
            return (do_matrix_ops<InstructionSet>(
                matrix_functions::instance().multiply_matrix_double,
                a.data(),
                b.data(),
                r.data(),
                size));
        } else if constexpr (std::is_same_v<DataType, float>) {
            return (do_matrix_ops<InstructionSet>(
                matrix_functions::instance().multiply_matrix_float,
                a.data(),
                b.data(),
                r.data(),
                size));
        } else if constexpr (std::is_same_v<DataType, int32_t>) {
            return (do_matrix_ops<InstructionSet>(
                matrix_functions::instance().multiply_matrix_int32,
                a.data(),
                b.data(),
                r.data(),
                size));
        } else if constexpr (std::is_same_v<DataType, int64_t>) {
            return (do_matrix_ops<InstructionSet>(
                matrix_functions::instance().multiply_matrix_int64,
                a.data(),
                b.data(),
                r.data(),
                size));
        }
    }
};

template <typename... Pairs>
std::variant<
    target_op_impl<typename Pairs::first_type, typename Pairs::second_type>...>
    as_target_op_variant(type_list<Pairs...>);

using target_op = decltype(as_target_op_variant(load_types{}));

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_TARGET_HPP_ */
