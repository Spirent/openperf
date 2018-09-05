#include <string.h>

#include "core/icp_common.h"

void *icp_aligned_realloc(void *ptr, size_t alignment, size_t size)
{
    assert(icp_is_power_of_two(alignment));

    void *to_return = NULL;

    /* If there is no current pointer, we can just make a new allocation */
    if (!ptr) {
        if (posix_memalign(&to_return, alignment, size) == 0) {
            memset(to_return, 0, size);
        }
        return (to_return);
    }

    /*
     * We need to resize an existing ptr.  See if realloc will keep the
     * alignment for us.
     */
    assert(icp_is_aligned(ptr, alignment));
    if ((to_return = realloc(ptr, size)) == NULL
        || icp_is_aligned(to_return, alignment)) {
        /*
         * Either the reallocation failed or it succeeded and still has the
         * proper alignment.  In either case, we just return our pointer.
         */
        return (to_return);
    }

    /*
     * We have a misaligned allocation.  Try to make a new aligned allocation
     * and copy the old contents to the new pointer.
     */
    void *misaligned = to_return;
    to_return = NULL;
    if (posix_memalign(&to_return, alignment, size) == 0) {
        /*
         * Copy original data into to_return pointer; misaligned is at least
         * size bytes long due to realloc above.
         */
        memcpy(to_return, misaligned, size);
        free(misaligned);
    } else {
        /*
         * Yikes!  We have a misaligned pointer and can't allocate a properly
         * aligned one.  I guess the misaligned pointer is better than nothing.
         */
        to_return = misaligned;
    }

    return (to_return);
}
