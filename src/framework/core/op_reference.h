#ifndef _OP_REFERENCE_H_
#define _OP_REFERENCE_H_

/**
 * @file
 */

#ifndef __cplusplus
#include <stdatomic.h>
#else
#include <atomic>
#define atomic_int std::atomic<int>
extern "C" {
#endif

/**
 * Embedable reference counting struct for shared
 * data structures.  Use the container_of macro to
 * find the parent structure from inside 'free'.
 */
struct op_reference
{
    void (*free)(const struct op_reference* ref);
    atomic_int count;
};

void op_reference_init(struct op_reference* ref,
                       void (*free)(const struct op_reference* ref));

/**
 * Increment the reference count
 */
void op_reference_retain(const struct op_reference* ref);

/**
 * Decrement the reference count.  Calls free when the
 * ref count hits zero.
 */
void op_reference_release(const struct op_reference* ref);

#ifdef __cplusplus
}
#endif

#endif /* _OP_REFERENCE_H_ */
