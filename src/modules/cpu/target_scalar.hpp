#ifndef _OP_CPU_TARGET_SCALAR_HPP_
#define _OP_CPU_TARGET_SCALAR_HPP_

#include <vector>
#include "target.hpp"

namespace openperf::cpu::internal {

template <class T> class target_scalar : public target
{
private:
    constexpr static size_t size = 32;

    std::vector<T> matrix_a;
    std::vector<T> matrix_b;
    std::vector<T> matrix_r;

public:
    target_scalar()
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
    }

    ~target_scalar() override = default;

    uint64_t operation() const override
    {
        auto& mx_r = const_cast<target_scalar<T>*>(this)->matrix_r;
        for (size_t i = 0; i < size; i++) {
            for (size_t j = 0; j < size; j++) {
                T sum = 0;
                for (size_t k = 0; k < size; k++) {
                    sum += matrix_a[i * size + k] * matrix_b[k * size + i];
                }
                mx_r[i * size + j] = sum;
            }
        }

        return 1;
    }
};

} // namespace openperf::cpu::internal

#endif // _OP_CPU_TARGET_SCALAR_HPP_
