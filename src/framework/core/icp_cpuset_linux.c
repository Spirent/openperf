#include "icp_cpuset.h"

#include <sched.h>
#include <unistd.h>

icp_cpuset_t icp_cpuset_create(void)
{
    cpu_set_t *cpuset = CPU_ALLOC(CPU_SETSIZE);
    if (cpuset) {
        CPU_ZERO_S(CPU_ALLOC_SIZE(CPU_SETSIZE), cpuset);
    }
    return cpuset;
}

void icp_cpuset_delete(icp_cpuset_t cpuset)
{
    if (!cpuset)
        return;
    CPU_FREE((cpu_set_t *) cpuset);
}

void icp_cpuset_clear(icp_cpuset_t cpuset)
{
    CPU_ZERO_S(CPU_ALLOC_SIZE(CPU_SETSIZE), cpuset);
}

size_t icp_cpuset_get_native_size(icp_cpuset_t cpuset  __attribute__((unused)))
{
    return CPU_ALLOC_SIZE(CPU_SETSIZE);
}

void *icp_cpuset_get_native_ptr(icp_cpuset_t cpuset)
{
    return (void *) cpuset;
}

void icp_cpuset_set(icp_cpuset_t cpuset, size_t cpu, bool val) 
{
    if (val)
        CPU_SET_S(cpu, CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)cpuset);
    else
        CPU_CLR_S(cpu, CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)cpuset);
}

bool icp_cpuset_get(icp_cpuset_t cpuset, size_t cpu)
{
    return CPU_ISSET_S(cpu, CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)cpuset);
}

size_t icp_cpuset_size(icp_cpuset_t cpuset __attribute__((unused)))
{
    return CPU_SETSIZE;
}

size_t icp_cpuset_count(icp_cpuset_t cpuset)
{
    return CPU_COUNT_S(CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)cpuset);
}

void icp_cpuset_and(icp_cpuset_t dest, icp_cpuset_t src)
{
    CPU_AND_S(CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)dest, (cpu_set_t *)dest, (cpu_set_t *) src);
}

void icp_cpuset_or(icp_cpuset_t dest, icp_cpuset_t src)
{
    CPU_OR_S(CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)dest, (cpu_set_t *)dest, (cpu_set_t *) src);
}

bool icp_cpuset_equal(icp_cpuset_t a, icp_cpuset_t b)
{
    return CPU_EQUAL_S(CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t *)a, (cpu_set_t *)b);
}

size_t icp_get_cpu_count(void)
{
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count < 0)
        return 0;
    return (size_t) count;
}

