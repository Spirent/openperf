#define DECLARE_MULTIPLICATE_MATRIX(type)                                      \
    export void multiplicate_matrix_##type (                                   \
        uniform type matrix_a[],                                               \
        uniform type matrix_b[],                                               \
        uniform type matrix_r[],                                               \
        uniform uint32 size)                                                   \
    {                                                                          \
        foreach (i = 0 ... size, j = 0 ... size) {                             \
            type sum = 0;                                                      \
            for (uniform uint32 k = 0; k < size; k++)                          \
                sum += matrix_a[i * size + k] * matrix_b[k * size + j];        \
            matrix_r[i * size + j] = sum;                                      \
        }                                                                      \
    }

DECLARE_MULTIPLICATE_MATRIX(float)
DECLARE_MULTIPLICATE_MATRIX(double)
DECLARE_MULTIPLICATE_MATRIX(uint32)
DECLARE_MULTIPLICATE_MATRIX(uint64)
