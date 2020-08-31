
#include <cinttypes>
#include "../matrix.hpp"

namespace openperf::cpu::internal::scalar {

template <typename T>
void multiplicate_matrix(T* matrix_a, T* matrix_b, T* matrix_r, uint32_t size)
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

void multiplicate_matrix_float(float* matrix_a,
                               float* matrix_b,
                               float* matrix_r,
                               uint32_t size)
{
    multiplicate_matrix<float>(matrix_a, matrix_b, matrix_r, size);
}

void multiplicate_matrix_double(double* matrix_a,
                                double* matrix_b,
                                double* matrix_r,
                                uint32_t size)
{
    multiplicate_matrix<double>(matrix_a, matrix_b, matrix_r, size);
}

void multiplicate_matrix_uint32(uint32_t* matrix_a,
                                uint32_t* matrix_b,
                                uint32_t* matrix_r,
                                uint32_t size)
{
    multiplicate_matrix<uint32_t>(matrix_a, matrix_b, matrix_r, size);
}

void multiplicate_matrix_uint64(uint64_t* matrix_a,
                                uint64_t* matrix_b,
                                uint64_t* matrix_r,
                                uint32_t size)
{
    multiplicate_matrix<uint64_t>(matrix_a, matrix_b, matrix_r, size);
}

} // namespace openperf::cpu::internal::scalar
