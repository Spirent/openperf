#include "core/op_reference.h"

void op_reference_init(struct op_reference* ref,
                       void (*free)(const struct op_reference* ref))
{
    ref->free = free;
    atomic_store_explicit(&ref->count, 1, memory_order_relaxed);
}

void op_reference_retain(const struct op_reference* ref)
{
    atomic_fetch_add_explicit(
        (atomic_int*)&ref->count, 1, memory_order_acq_rel);
}

void op_reference_release(const struct op_reference* ref)
{
    /* Note: atomic_fetch_sub returns _previous_ value */
    if (atomic_fetch_sub_explicit(
            (atomic_int*)&ref->count, 1, memory_order_acq_rel)
        == 1) {
        ref->free(ref);
    }
}
