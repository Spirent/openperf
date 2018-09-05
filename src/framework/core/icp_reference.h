#ifndef _ICP_REFERENCE_H_
#define _ICP_REFERENCE_H_

#include <stdatomic.h>

/**
 * Embedable reference counting struct for shared
 * data structures.  Use the container_of macro to
 * find the parent structure from inside 'free'.
 */
struct icp_reference {
    void (*free)(const struct icp_reference *ref);
    atomic_int count;
};

void icp_reference_init(struct icp_reference *ref,
                        void (*free)(const struct icp_reference *ref));

/**
 * Increment the reference count
 */
void icp_reference_retain(const struct icp_reference *ref);

/**
 * Decrement the reference count.  Calls free when the
 * ref count hits zero.
 */
void icp_reference_release(const struct icp_reference *ref);

#endif /* _ICP_REFERENCE_H_ */
