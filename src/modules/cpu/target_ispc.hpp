#ifndef _OP_CPU_TARGET_ISPC_HPP_
#define _OP_CPU_TARGET_ISPC_HPP_

#include <vector>
#include "target.hpp"
#include "matrix.hpp"

namespace openperf::cpu::internal {

template <class T> class target_ispc : public target
{
    using function_t = void (*)(T*, T*, T*, uint32_t);

private:
    constexpr static size_t size = 32;

    std::vector<T> matrix_a;
    std::vector<T> matrix_b;
    std::vector<T> matrix_r;
    function_t m_operation;

public:
    target_ispc(cpu::instruction_set iset)
    {
        matrix_a.resize(size * size);
        matrix_b.resize(size * size);
        matrix_r.resize(size * size);

        for (size_t i = 0; i < size * size; ++i) {
            auto row = i / size;
            auto col = i % size;
            matrix_a[i] = row * col;
            matrix_b[i] = static_cast<T>(size * size) / (row * col + 1);
        }

        auto functions = function_wrapper<function_t>();
        m_operation = functions.function(iset);

        if (m_operation == nullptr)
            throw std::runtime_error("Instruction set not supported");
    }

    ~target_ispc() override = default;

    uint64_t operation() const override
    {
        auto cthis = const_cast<target_ispc*>(this);
        m_operation(cthis->matrix_a.data(),
                    cthis->matrix_b.data(),
                    cthis->matrix_r.data(),
                    size);
        return 1;
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_ISPC_HPP_
