#ifndef _OP_CPU_HPP_
#define _OP_CPU_HPP_

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Get CPU architecture name
 *
 *  @return
 *    The architecture name.
 */
const char* op_cpu_architecture(void);

/**
 *  Get CPU L1 cache line size
 *
 *  @return
 *    The size in bytes or 0 if fail.
 */
uint16_t op_cpu_l1_cache_line_size(void);

/**
 * Get number of CPUs.
 *
 * @return
 *   The number of CPUs or 0 if fail.
 */
size_t op_cpu_count(void);

#ifdef __cplusplus
}
#endif

#endif // _OP_CPU_HPP_
