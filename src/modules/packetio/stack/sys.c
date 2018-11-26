#include "lwip/sys.h"
#include <time.h>

#include "core/icp_core.h"

/**
 * Return the number of milliseconds since startup.
 *
 * TODO: Use timesync from inception
 */
uint32_t sys_now()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        icp_exit("clock_gettime call failed");
    }

    return ((ts.tv_sec * 1000000 + ts.tv_nsec / 1000) % 0xffffffff);
}
