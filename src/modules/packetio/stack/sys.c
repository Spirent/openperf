#include "lwip/sys.h"
#include <time.h>

#include "core/op_core.h"

/**
 * Return the number of milliseconds since startup.
 *
 * TODO: Use timesync from openperf
 */
uint32_t sys_now()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        op_exit("clock_gettime call failed");
    }

    return ((ts.tv_sec * 1000000 + ts.tv_nsec / 1000) % 0xffffffff);
}
