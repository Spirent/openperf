#define DECLARE_MULTIPLY_MATRIX(type)                                          \
    export void multiply_matrix_##type (                                       \
        uniform type matrix_a[],                                               \
        uniform type matrix_b[],                                               \
        uniform type matrix_r[],                                               \
        uniform uint32 size)                                                   \
    {                                                                          \
        foreach(i = 0 ... size, j = 0 ... size)                                \
        {                                                                      \
            type sum = 0;                                                      \
            for (uniform uint32 k = 0; k < size; k++)                          \
                sum += matrix_a[i * size + k] * matrix_b[k * size + j];        \
            matrix_r[i * size + j] = sum;                                      \
        }                                                                      \
    }

DECLARE_MULTIPLY_MATRIX(float)
DECLARE_MULTIPLY_MATRIX(double)
DECLARE_MULTIPLY_MATRIX(uint32)
DECLARE_MULTIPLY_MATRIX(uint64)
