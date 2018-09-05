#ifndef _ICP_COMMON_H_
#define _ICP_COMMON_H_

/**
 * @file
 */

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
#define icp_safe_log(fmt, ...) ({                                       \
            printf("%s:%s:%d: " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
            fflush(stdout);                                             \
        })

/**
 * Use C11 generics to provide proper min/max functions.
 */
#define ICP_MIN(type, name)                                             \
    inline __attribute__((always_inline))                               \
    type icp_min_ ## name(const type x, const type y)                   \
    {                                                                   \
        return (x < y ? x : y);                                         \
    }

ICP_MIN(short, s)
ICP_MIN(unsigned short, us)
ICP_MIN(int, i)
ICP_MIN(unsigned, u)
ICP_MIN(long, l)
ICP_MIN(unsigned long, ul)
ICP_MIN(long long, ll)
ICP_MIN(unsigned long long, ull)
ICP_MIN(float, f)
ICP_MIN(double, d)

#undef ICP_MIN

#define icp_min(x, y) \
    _Generic((x),     \
             short:icp_min_s,                   \
             unsigned short: icp_min_us,        \
             int: icp_min_i,                    \
             unsigned: icp_min_u,               \
             long: icp_min_l,                   \
             unsigned long: icp_min_ul,         \
             long long: icp_min_ll,             \
             unsigned long long: icp_min_ull,   \
             float: icp_min_f,                  \
             double: icp_min_d                  \
        )(x, y)

#define ICP_MAX(type, name)                                             \
    inline __attribute__((always_inline))                               \
    type icp_max_ ## name(const type x, const type y)                   \
    {                                                                   \
        return (x > y ? x : y);                                         \
    }

ICP_MAX(short, s)
ICP_MAX(unsigned short, us)
ICP_MAX(int, i)
ICP_MAX(unsigned, u)
ICP_MAX(long, l)
ICP_MAX(unsigned long, ul)
ICP_MAX(long long, ll)
ICP_MAX(unsigned long long, ull)
ICP_MAX(float, f)
ICP_MAX(double, d)

#undef ICP_MAX

#define icp_max(x, y) \
    _Generic((x),     \
             short:icp_max_s,                   \
             unsigned short: icp_max_us,        \
             int: icp_max_i,                    \
             unsigned: icp_max_u,               \
             long: icp_max_l,                   \
             unsigned long: icp_max_ul,         \
             long long: icp_max_ll,             \
             unsigned long long: icp_max_ull,   \
             float: icp_max_f,                  \
             double: icp_max_d                  \
        )(x, y)

/**
 * Macro to return the size of an array
 */
#define icp_count_of(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

/**
 * Also provide some generic power of two and alignment functions
 */
#define ICP_POWER_OF_TWO(type, name)            \
    inline __attribute__((always_inline))       \
    bool icp_is_power_of_two_ ## name(type x)   \
    {                                           \
        return ((x & (x - 1)) == 0);            \
    }

ICP_POWER_OF_TWO(unsigned short, us)
ICP_POWER_OF_TWO(unsigned, u)
ICP_POWER_OF_TWO(unsigned long, ul)
ICP_POWER_OF_TWO(unsigned long long, ull)

#undef ICP_POWER_OF_TWO

#define icp_is_power_of_two(x)                              \
    _Generic((x),                                           \
             unsigned short: icp_is_power_of_two_us,        \
             unsigned: icp_is_power_of_two_u,               \
             unsigned long: icp_is_power_of_two_ul,         \
             unsigned long long: icp_is_power_of_two_ull    \
        )(x)

#define ICP_ALIGNED(type, name)                             \
    inline __attribute__((always_inline))                   \
    bool icp_is_aligned_ ## name(void *ptr, type alignment) \
    {                                                       \
        assert(icp_is_power_of_two(alignment));             \
        return (((uintptr_t)ptr & (alignment - 1)) == 0);   \
    }

ICP_ALIGNED(unsigned short, us)
ICP_ALIGNED(unsigned, u)
ICP_ALIGNED(unsigned long, ul)
ICP_ALIGNED(unsigned long long, ull)

#undef ICP_POWER_OF_TWO

#define icp_is_aligned(ptr, alignment)                      \
    _Generic((alignment),                                   \
             unsigned short: icp_is_aligned_us,             \
             unsigned: icp_is_aligned_u,                    \
             unsigned long: icp_is_aligned_ul,              \
             unsigned long long: icp_is_aligned_ull         \
        )(ptr, alignment)

/**
 * Kill the app and log a message to the console
 *
 * @param[in] format
 *   The printf format string, followed by variable arguments
 */
void icp_exit(const char *format, ...)
    __attribute__((noreturn))
    __attribute__((format(printf, 1, 2)));

#define icp_assert(condition, fmt, ...)         \
    do {                                        \
        if (!(condition)) {                     \
            icp_exit(fmt, ##__VA_ARGS__);       \
        }                                       \
    } while (0)

#endif
