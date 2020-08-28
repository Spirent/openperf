
#include <cinttypes>
#include "../matrix.hpp"

namespace openperf::cpu::internal::scalar {

#define DEFINE_MULTIPLICATE_MATRIX(suffix, type)                               \
    void multiplicate_matrix_##suffix(                                         \
        type* matrix_a, type* matrix_b, type* matrix_r, uint32_t size)         \
    {                                                                          \
        for (uint32_t i = 0; i < size; i++) {                                  \
            for (uint32_t j = 0; j < size; j++) {                              \
                type sum = 0;                                                  \
                for (uint32_t k = 0; k < size; k++)                            \
                    sum += matrix_a[i * size + k] * matrix_b[k * size + j];    \
                matrix_r[i * size + j] = sum;                                  \
            }                                                                  \
        }                                                                      \
    }

DEFINE_MULTIPLICATE_MATRIX(float, float)
DEFINE_MULTIPLICATE_MATRIX(double, double)
DEFINE_MULTIPLICATE_MATRIX(uint32, uint32_t)
DEFINE_MULTIPLICATE_MATRIX(uint64, uint64_t)

} // namespace openperf::cpu::internal::scalar
