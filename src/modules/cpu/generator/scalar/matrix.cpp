#include <cinttypes>
#include "cpu/generator/matrix_functions.hpp"

namespace scalar {

template <typename T>
void multiply_matrix(const T matrix_a[],
                     const T matrix_b[],
                     T matrix_r[],
                     size_t size)
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
                           size_t size)
{
    multiply_matrix(matrix_a, matrix_b, matrix_r, size);
}

void multiply_matrix_double(const double matrix_a[],
                            const double matrix_b[],
                            double matrix_r[],
                            size_t size)
{
    multiply_matrix(matrix_a, matrix_b, matrix_r, size);
}

void multiply_matrix_int32(const int32_t matrix_a[],
                           const int32_t matrix_b[],
                           int32_t matrix_r[],
                           size_t size)
{
    multiply_matrix(matrix_a, matrix_b, matrix_r, size);
}

void multiply_matrix_int64(const int64_t matrix_a[],
                           const int64_t matrix_b[],
                           int64_t matrix_r[],
                           size_t size)
{
    multiply_matrix(matrix_a, matrix_b, matrix_r, size);
}

} // namespace scalar
