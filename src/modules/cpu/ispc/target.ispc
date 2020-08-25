
#define DECLARE_MULTIPLICATE_MATRIX(type)                                      \
    export uniform uint64 multiplicate_matrix_##type (                         \
        uniform type* uniform matrix_a,                                        \
        uniform type* uniform matrix_b,                                        \
        uniform uint16 size)                                                   \
    {                                                                          \
        for (uint16 i = 0; i < size; ++i) {                                    \
            for (uint16 j = 0; j < size; ++j) {                                \
                type sum = 0;                                                  \
                for (uint16 k = 0; k < size; ++k) {                            \
                    sum += matrix_a[i * size + k] * matrix_b[k * size + j];    \
                }                                                              \
            }                                                                  \
        }                                                                      \
        return size * size * size;                                             \
    }

#define DECLARE_PARALLEL_MULTIPLICATE_MATRIX(type)                             \
    export uniform uint64 multiplicate_matrix_##type (                         \
        uniform type* uniform matrix_a,                                        \
        uniform type* uniform matrix_b,                                        \
        uniform uint16 size)                                                   \
    {                                                                          \
        foreach (i = 0 ... size, j = 0 ... size) {                             \
            type sum = 0;                                                      \
            for (uint16 k = 0; k < size; ++k) {                                \
                sum += matrix_a[i * size + k] * matrix_b[k * size + j];        \
            }                                                                  \
        }                                                                      \
        return size * size * size;                                             \
    }

DECLARE_PARALLEL_MULTIPLICATE_MATRIX(uint32)
DECLARE_PARALLEL_MULTIPLICATE_MATRIX(uint64)
DECLARE_PARALLEL_MULTIPLICATE_MATRIX(float)
DECLARE_PARALLEL_MULTIPLICATE_MATRIX(double)
