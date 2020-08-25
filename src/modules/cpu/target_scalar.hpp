#ifndef _OP_CPU_TARGET_SCALAR_HPP_
#define _OP_CPU_TARGET_SCALAR_HPP_

#include <vector>
#include "target.hpp"

namespace openperf::cpu::internal {

template <class T> class target_scalar : public target
{
private:
    using square_matrix = std::vector<std::vector<T>>;

    constexpr static size_t size = 32;

    square_matrix matrix1;
    square_matrix matrix2;

public:
    target_scalar()
    {
        matrix1.resize(size);
        matrix2.resize(size);
        for (size_t i = 0; i < size; ++i) {
            matrix1[i].resize(size);
            matrix2[i].resize(size);
            for (size_t j = 0; j < size; ++j) {
                matrix1[i][j] = i * j;
                matrix2[i][j] = static_cast<T>(size * size) / (i * j + 1);
            }
        }
    }

    ~target_scalar() override = default;

    [[clang::optnone]] uint64_t operation() const override
    {
        for (size_t i = 0; i < size; ++i) {
            for (size_t j = 0; j < size; ++j) {
                T sum = 0;
                for (size_t k = 0; k < size; ++k) {
                    sum += matrix1[i][k] * matrix2[k][j];
                }
            }
        }

        return size * size * size;
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_SCALAR_HPP_
