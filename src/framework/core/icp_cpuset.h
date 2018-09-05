#ifndef _ICP_CPUSET_H_
#define _ICP_CPUSET_H_

#include <stddef.h>
#include <stdbool.h>

typedef void * icp_cpuset_t;

icp_cpuset_t icp_cpuset_create(void);

int icp_cpuset_from_string(icp_cpuset_t cpuset, const char *str);

void icp_cpuset_to_string(icp_cpuset_t cpuset, char *buf, size_t buflen);

void icp_cpuset_delete(icp_cpuset_t cpuset);

void icp_cpuset_clear(icp_cpuset_t cpuset);

size_t icp_cpuset_get_native_size(icp_cpuset_t cpuset);

void *icp_cpuset_get_native_ptr(icp_cpuset_t cpuset);

void icp_cpuset_set(icp_cpuset_t cpuset, size_t cpu, bool val);

bool icp_cpuset_get(icp_cpuset_t cpuset, size_t cpu);

size_t icp_cpuset_size(icp_cpuset_t cpuset);

size_t icp_cpuset_count(icp_cpuset_t cpuset);

void icp_cpuset_and(icp_cpuset_t dest, icp_cpuset_t src);

void icp_cpuset_or(icp_cpuset_t dest, icp_cpuset_t src);

bool icp_cpuset_equal(icp_cpuset_t a, icp_cpuset_t b);

/**
 * Set all bits in specified range to specified value.
 *
 * @param cpuset
 *   The cpuset to modify.
 * @param start
 *   The starting cpu id.
 * @param len
 *   The length of the range to set.
 * @param val
 *   The value to set for each bit in the range.
 */
inline void icp_cpuset_set_range(icp_cpuset_t cpuset, size_t start, size_t len, bool val)
{
    size_t i, end = start + len;
    for (i = start; i < end; ++i) {
        icp_cpuset_set(cpuset, i, val);
    }
}

/**
 * Create cpuset from string.
 *
 * @param str
 *   The string representing the cpuset.
 * @return
 *   The cpuset if successful or NULL if fail.
 */
inline icp_cpuset_t icp_cpuset_create_from_string(const char *str)
{
    icp_cpuset_t cpuset = icp_cpuset_create();
    if (cpuset) {
        if (icp_cpuset_from_string(cpuset, str) != 0) {
            icp_cpuset_delete(cpuset);
            cpuset = NULL;
        }
    }
    return cpuset;
}

/**
 * Find the first cpu in the cpuset.
 *
 * @param[in] cpuset
 *   The cpuset.
 * @param[out] cpu
 *   The index of the cpu.
 * @return
 *   0 if successful, non-zero if fail.
 */
inline int icp_cpuset_get_first(icp_cpuset_t cpuset, size_t *cpu)
{
    size_t i, size = icp_cpuset_size(cpuset);
    for (i = 0; i < size; ++i) {
        if (icp_cpuset_get(cpuset, i)) {
            (*cpu) = i;
            return 0;
        }
    }
    return -1;
}

/**
 * Find the next cpu in the cpuset.
 *
 * @param[in] cpuset
 *   The cpuset.
 * @param cpu
 *   The index of the cpu.
 * @return
 *   0 if successful, non-zero if fail.
 */
inline int icp_cpuset_get_next(icp_cpuset_t cpuset, size_t *cpu)
{
    int i, size = icp_cpuset_size(cpuset);
    for (i = (*cpu) + 1; i < size; ++i) {
        if (icp_cpuset_get(cpuset, i)) {
            (*cpu) = i;
            return 0;
        }
    }
    return -1;
}

/**
 * Get number of cpus.
 *
 * @return
 *   The number of cpus or 0 if fail.
 */
size_t icp_get_cpu_count(void);

#endif /* _ICP_CPUSET_H_ */
