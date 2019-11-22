#include <sys/param.h>
#include <sys/cpuset.h>
#include <sys/sysctl.h>

#include "op_cpuset.h"

op_cpuset_t op_cpuset_create(void)
{
    cpuset_t* cpuset = (cpuset_t*)calloc(1, sizeof(cpuset_t));
    if (cpuset) { CPU_ZERO(cpuset); }
    return cpuset;
}

void op_cpuset_delete(op_cpuset_t cpuset)
{
    if (!cpuset) return;
    free((cpuset_t*)cpuset);
}

void op_cpuset_clear(op_cpuset_t cpuset) { CPU_ZERO((cpuset_t*)cpuset); }

size_t op_cpuset_get_native_size(op_cpuset_t cpuset __attribute__((unused)))
{
    return sizeof(cpuset_t);
}

void* op_cpuset_get_native_ptr(op_cpuset_t cpuset) { return (void*)cpuset; }

void op_cpuset_set(op_cpuset_t cpuset, size_t cpu, bool val)
{
    if (val)
        CPU_SET(cpu, (cpuset_t*)cpuset);
    else
        CPU_CLR(cpu, (cpuset_t*)cpuset);
}

bool op_cpuset_get(op_cpuset_t cpuset, size_t cpu)
{
    return CPU_ISSET(cpu, (cpuset_t*)cpuset);
}

size_t op_cpuset_size(op_cpuset_t cpuset __attribute__((unused)))
{
    return MAXCPU;
}

size_t op_cpuset_count(op_cpuset_t cpuset)
{
    return CPU_COUNT((cpuset_t*)cpuset);
}

void op_cpuset_and(op_cpuset_t dest, op_cpuset_t src)
{
    CPU_AND((cpuset_t*)dest, (cpuset_t*)src);
}

void op_cpuset_or(op_cpuset_t dest, op_cpuset_t src)
{
    CPU_OR((cpuset_t*)dest, (cpuset_t*)src);
}

bool op_cpuset_equal(op_cpuset_t a, op_cpuset_t b)
{
    return CPU_CMP((cpuset_t*)a, (cpuset_t*)b) == 0;
}

size_t op_get_cpu_count(void)
{
    static const int mib[] = {CTL_HW, HW_NCPU};
    uint64_t nb_cpus = 0;
    size_t length = sizeof(nb_cpus);
    int err = 0;

    if ((err = sysctl(mib, 2, &nb_cpus, &length, NULL, 0)) != 0) { return 0; }

    return (size_t)nb_cpus;
}
