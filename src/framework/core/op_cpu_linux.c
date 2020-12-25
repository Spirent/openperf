#include "op_cpu.h"

#include <unistd.h>
#include <sys/utsname.h>

uint16_t op_cpu_l1_cache_line_size()
{
    return sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
}

const char* op_cpu_architecture()
{
    static struct utsname info;
    uname(&info);
    return info.machine;
}

size_t op_cpu_count(void)
{
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count < 0) return 0;
    return (size_t)count;
}
