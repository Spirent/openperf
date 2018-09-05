#include <sys/param.h>
#include <sys/cpuset.h>
#include <sys/sysctl.h>

#include "icp_cpuset.h"

icp_cpuset_t icp_cpuset_create(void)
{
    cpuset_t *cpuset = (cpuset_t *) calloc(1, sizeof(cpuset_t));
    if (cpuset) {
        CPU_ZERO(cpuset);
    }
    return cpuset;
}

void icp_cpuset_delete(icp_cpuset_t cpuset)
{
    if (!cpuset)
        return;
    free((cpuset_t *) cpuset);
}

void icp_cpuset_clear(icp_cpuset_t cpuset)
{
    CPU_ZERO((cpuset_t *) cpuset);
}

size_t icp_cpuset_get_native_size(icp_cpuset_t cpuset __attribute__((unused)))
{
    return sizeof(cpuset_t);
}

void *icp_cpuset_get_native_ptr(icp_cpuset_t cpuset)
{
    return (void *) cpuset;
}

void icp_cpuset_set(icp_cpuset_t cpuset, size_t cpu, bool val)
{
    if (val)
        CPU_SET(cpu, (cpuset_t *)cpuset);
    else
        CPU_CLR(cpu, (cpuset_t *)cpuset);
}

bool icp_cpuset_get(icp_cpuset_t cpuset, size_t cpu)
{
    return CPU_ISSET(cpu, (cpuset_t *)cpuset);
}

size_t icp_cpuset_size(icp_cpuset_t cpuset __attribute__((unused)))
{
    return MAXCPU;
}

size_t icp_cpuset_count(icp_cpuset_t cpuset)
{
    return CPU_COUNT((cpuset_t *)cpuset);
}

void icp_cpuset_and(icp_cpuset_t dest, icp_cpuset_t src)
{
    CPU_AND((cpuset_t *)dest, (cpuset_t *)src);
}

void icp_cpuset_or(icp_cpuset_t dest, icp_cpuset_t src)
{
    CPU_OR((cpuset_t *)dest, (cpuset_t *)src);
}

bool icp_cpuset_equal(icp_cpuset_t a, icp_cpuset_t b)
{
    return CPU_CMP((cpuset_t *)a, (cpuset_t *)b) == 0;
}

size_t icp_get_cpu_count(void)
{
    static const int mib[] = { CTL_HW, HW_NCPU };
    uint64_t nb_cpus = 0;
    size_t length = sizeof(nb_cpus);
    int err = 0;

    if ((err = sysctl(mib, 2, &nb_cpus, &length, NULL, 0)) != 0) {
        return 0;
    }

    return (size_t) nb_cpus;
}

