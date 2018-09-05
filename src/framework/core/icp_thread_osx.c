#include <pthread.h>

#include "core/icp_thread.h"

int icp_thread_setname(const char *name)
{
    return pthread_setname_np(name);
}

int icp_thread_getname(pthread_t tid, char *name)
{
    return (pthread_getname_np(tid, name, ICP_THREAD_NAME_MAX_LENGTH));
}

int icp_thread_set_affinity(int core_id __attribute__((unused)))
{
    return (-1);
}

int icp_thread_set_affinity_mask(const icp_cpu_mask_t *cpu_mask __attribute__((unused)))
{
    return (-1);
}

int icp_thread_get_affinity_mask(icp_cpu_mask_t *cpu_mask __attribute__((unused)))
{
    return (-1);
}

