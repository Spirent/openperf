#include <pthread_np.h>

#include "core/icp_thread.h"

int icp_thread_setname(const char *name)
{
    pthread_set_name_np(pthread_self(), name);
    return (0);
}

int icp_thread_getname(pthread_t tid __attribute__((unused)),
                       char *name __attribute__((unused)))
{
    return (-1);
}

int icp_thread_set_affinity(int core_id __attribute__((unused)))
{
    return (-1);
}

int icp_thread_set_affinity_mask(const icp_cpuset_t cpuset __attribute__((unused)))
{
    return (-1);
}

int icp_thread_get_affinity_mask(icp_cpuset_t cpuset __attribute__((unused)))
{
    return (-1);
}
