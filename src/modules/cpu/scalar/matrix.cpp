#include <cinttypes>
#include "cpu/matrix.hpp"

namespace scalar {

template <typename T>
void multiply_matrix(const T matrix_a[],
                     const T matrix_b[],
                     T matrix_r[],
                     uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        for (uint32_t j = 0; j < size; j++) {
            T sum = 0;
            for (uint32_t k = 0; k < size; k++)
                sum += matrix_a[i * size + k] * matrix_b[k * size + j];
            matrix_r[i * size + j] = sum;
        }
    }
}

void multiply_matrix_float(const float matrix_a[],
                           const float matrix_b[],
                           float matrix_r[],
                           uint32_t size)
{
    multiply_matrix<float>(matrix_a, matrix_b, matrix_r, size);
}

void multiply_matrix_double(const double matrix_a[],
                            const double matrix_b[],
                            double matrix_r[],
                            uint32_t size)
{
    multiply_matrix<double>(matrix_a, matrix_b, matrix_r, size);
}

void multiply_matrix_uint32(const uint32_t matrix_a[],
                            const uint32_t matrix_b[],
                            uint32_t matrix_r[],
                            uint32_t size)
{
    multiply_matrix<uint32_t>(matrix_a, matrix_b, matrix_r, size);
}

void multiply_matrix_uint64(const uint64_t matrix_a[],
                            const uint64_t matrix_b[],
                            uint64_t matrix_r[],
                            uint32_t size)
{
    multiply_matrix<uint64_t>(matrix_a, matrix_b, matrix_r, size);
}

} // namespace scalar
