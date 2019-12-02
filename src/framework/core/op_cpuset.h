#ifndef _OP_CPUSET_H_
#define _OP_CPUSET_H_

#include <stddef.h>
#include <stdbool.h>

typedef void* op_cpuset_t;

op_cpuset_t op_cpuset_create(void);

int op_cpuset_from_string(op_cpuset_t cpuset, const char* str);

void op_cpuset_to_string(op_cpuset_t cpuset, char* buf, size_t buflen);

void op_cpuset_delete(op_cpuset_t cpuset);

void op_cpuset_clear(op_cpuset_t cpuset);

size_t op_cpuset_get_native_size(op_cpuset_t cpuset);

void* op_cpuset_get_native_ptr(op_cpuset_t cpuset);

void op_cpuset_set(op_cpuset_t cpuset, size_t cpu, bool val);

bool op_cpuset_get(op_cpuset_t cpuset, size_t cpu);

size_t op_cpuset_size(op_cpuset_t cpuset);

size_t op_cpuset_count(op_cpuset_t cpuset);

void op_cpuset_and(op_cpuset_t dest, op_cpuset_t src);

void op_cpuset_or(op_cpuset_t dest, op_cpuset_t src);

bool op_cpuset_equal(op_cpuset_t a, op_cpuset_t b);

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
inline void
op_cpuset_set_range(op_cpuset_t cpuset, size_t start, size_t len, bool val)
{
    size_t i, end = start + len;
    for (i = start; i < end; ++i) { op_cpuset_set(cpuset, i, val); }
}

/**
 * Create cpuset from string.
 *
 * @param str
 *   The string representing the cpuset.
 * @return
 *   The cpuset if successful or NULL if fail.
 */
inline op_cpuset_t op_cpuset_create_from_string(const char* str)
{
    op_cpuset_t cpuset = op_cpuset_create();
    if (cpuset) {
        if (op_cpuset_from_string(cpuset, str) != 0) {
            op_cpuset_delete(cpuset);
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
inline int op_cpuset_get_first(op_cpuset_t cpuset, size_t* cpu)
{
    size_t i, size = op_cpuset_size(cpuset);
    for (i = 0; i < size; ++i) {
        if (op_cpuset_get(cpuset, i)) {
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
inline int op_cpuset_get_next(op_cpuset_t cpuset, size_t* cpu)
{
    int i, size = op_cpuset_size(cpuset);
    for (i = (*cpu) + 1; i < size; ++i) {
        if (op_cpuset_get(cpuset, i)) {
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
size_t op_get_cpu_count(void);

#endif /* _OP_CPUSET_H_ */
