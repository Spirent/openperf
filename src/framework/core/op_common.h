#ifndef _OP_COMMON_H_
#define _OP_COMMON_H_

/**
 * @file
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Various useful macros and small inline functions
 */

/**
 * Provides a safe way to get information to the console
 * independent of context
 */
#define op_safe_log(fmt, ...) ({                                        \
            printf("%s:%s:%d: " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
            fflush(stdout);                                             \
        })

/**
 * Use C11 generics to provide proper min/max functions.
 */
#define OP_MIN(type, name)                              \
    inline __attribute__((always_inline))               \
    type op_min_ ## name(const type x, const type y)    \
    {                                                   \
        return (x < y ? x : y);                         \
    }

OP_MIN(short, s)
OP_MIN(unsigned short, us)
OP_MIN(int, i)
OP_MIN(unsigned, u)
OP_MIN(long, l)
OP_MIN(unsigned long, ul)
OP_MIN(long long, ll)
OP_MIN(unsigned long long, ull)
OP_MIN(float, f)
OP_MIN(double, d)

#undef OP_MIN

#define op_min(x, y)                            \
    _Generic((x),                               \
             short:op_min_s,                    \
             unsigned short: op_min_us,         \
             int: op_min_i,                     \
             unsigned: op_min_u,                \
             long: op_min_l,                    \
             unsigned long: op_min_ul,          \
             long long: op_min_ll,              \
             unsigned long long: op_min_ull,    \
             float: op_min_f,                   \
             double: op_min_d                   \
        )(x, y)

#define OP_MAX(type, name)                              \
    inline __attribute__((always_inline))               \
    type op_max_ ## name(const type x, const type y)    \
    {                                                   \
        return (x > y ? x : y);                         \
    }

OP_MAX(short, s)
OP_MAX(unsigned short, us)
OP_MAX(int, i)
OP_MAX(unsigned, u)
OP_MAX(long, l)
OP_MAX(unsigned long, ul)
OP_MAX(long long, ll)
OP_MAX(unsigned long long, ull)
OP_MAX(float, f)
OP_MAX(double, d)

#undef OP_MAX

#define op_max(x, y)                            \
    _Generic((x),                               \
             short:op_max_s,                    \
             unsigned short: op_max_us,         \
             int: op_max_i,                     \
             unsigned: op_max_u,                \
             long: op_max_l,                    \
             unsigned long: op_max_ul,          \
             long long: op_max_ll,              \
             unsigned long long: op_max_ull,    \
             float: op_max_f,                   \
             double: op_max_d                   \
        )(x, y)

/**
 * Macro to return the size of an array
 */
#define op_count_of(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

/**
 * Also provide some generic power of two and alignment functions
 */
#define OP_POWER_OF_TWO(type, name)             \
    inline __attribute__((always_inline))       \
    bool op_is_power_of_two_ ## name(type x)    \
    {                                           \
        return ((x & (x - 1)) == 0);            \
    }

OP_POWER_OF_TWO(unsigned short, us)
OP_POWER_OF_TWO(unsigned, u)
OP_POWER_OF_TWO(unsigned long, ul)
OP_POWER_OF_TWO(unsigned long long, ull)

#undef OP_POWER_OF_TWO

#define op_is_power_of_two(x)                           \
    _Generic((x),                                       \
             unsigned short: op_is_power_of_two_us,     \
             unsigned: op_is_power_of_two_u,            \
             unsigned long: op_is_power_of_two_ul,      \
             unsigned long long: op_is_power_of_two_ull \
        )(x)

#define OP_ALIGNED(type, name)                              \
    inline __attribute__((always_inline))                   \
    bool op_is_aligned_ ## name(void *ptr, type alignment)  \
    {                                                       \
        assert(op_is_power_of_two(alignment));              \
        return (((uintptr_t)ptr & (alignment - 1)) == 0);   \
    }

OP_ALIGNED(unsigned short, us)
OP_ALIGNED(unsigned, u)
OP_ALIGNED(unsigned long, ul)
OP_ALIGNED(unsigned long long, ull)

#undef OP_POWER_OF_TWO

#define op_is_aligned(ptr, alignment)               \
    _Generic((alignment),                           \
             unsigned short: op_is_aligned_us,      \
             unsigned: op_is_aligned_u,             \
             unsigned long: op_is_aligned_ul,       \
             unsigned long long: op_is_aligned_ull  \
        )(ptr, alignment)

/**
 * Kill the app and log a message to the console
 *
 * @param[in] format
 *   The printf format string, followed by variable arguments
 */
    void op_exit(const char *format, ...)
    __attribute__((noreturn))
    __attribute__((format(printf, 1, 2)));

#define op_assert(condition, fmt, ...)          \
    do {                                        \
        if (!(condition)) {                     \
            op_exit(fmt, ##__VA_ARGS__);        \
        }                                       \
    } while (0)

    void op_init_crash_handler();

#ifdef __cplusplus
}
#endif

#endif
