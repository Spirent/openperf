#include "op_cpuset.h"

#include <sched.h>
#include <unistd.h>

op_cpuset_t op_cpuset_create(void)
{
    cpu_set_t* cpuset = CPU_ALLOC(CPU_SETSIZE);
    if (cpuset) { CPU_ZERO_S(CPU_ALLOC_SIZE(CPU_SETSIZE), cpuset); }
    return cpuset;
}

void op_cpuset_delete(op_cpuset_t cpuset)
{
    if (!cpuset) return;
    CPU_FREE((cpu_set_t*)cpuset);
}

void op_cpuset_clear(op_cpuset_t cpuset)
{
    CPU_ZERO_S(CPU_ALLOC_SIZE(CPU_SETSIZE), cpuset);
}

size_t op_cpuset_get_native_size(op_cpuset_t cpuset __attribute__((unused)))
{
    return CPU_ALLOC_SIZE(CPU_SETSIZE);
}

void* op_cpuset_get_native_ptr(op_cpuset_t cpuset) { return (void*)cpuset; }

void op_cpuset_set(op_cpuset_t cpuset, size_t cpu, bool val)
{
    if (val)
        CPU_SET_S(cpu, CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t*)cpuset);
    else
        CPU_CLR_S(cpu, CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t*)cpuset);
}

bool op_cpuset_get(op_cpuset_t cpuset, size_t cpu)
{
    return CPU_ISSET_S(cpu, CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t*)cpuset);
}

size_t op_cpuset_size(op_cpuset_t cpuset __attribute__((unused)))
{
    return CPU_SETSIZE;
}

size_t op_cpuset_count(op_cpuset_t cpuset)
{
    return CPU_COUNT_S(CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t*)cpuset);
}

void op_cpuset_and(op_cpuset_t dest, op_cpuset_t src)
{
    CPU_AND_S(CPU_ALLOC_SIZE(CPU_SETSIZE),
              (cpu_set_t*)dest,
              (cpu_set_t*)dest,
              (cpu_set_t*)src);
}

void op_cpuset_or(op_cpuset_t dest, op_cpuset_t src)
{
    CPU_OR_S(CPU_ALLOC_SIZE(CPU_SETSIZE),
             (cpu_set_t*)dest,
             (cpu_set_t*)dest,
             (cpu_set_t*)src);
}

bool op_cpuset_equal(op_cpuset_t a, op_cpuset_t b)
{
    return CPU_EQUAL_S(
        CPU_ALLOC_SIZE(CPU_SETSIZE), (cpu_set_t*)a, (cpu_set_t*)b);
}
