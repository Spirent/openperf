#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "core/icp_common.h"
#include "core/icp_thread.h"

int icp_thread_setname(const char *name)
{
    return pthread_setname_np(pthread_self(), name);
}

int icp_thread_getname(pthread_t tid, char *name)
{
    return (pthread_getname_np(tid, name, ICP_THREAD_NAME_MAX_LENGTH));
}

#include <stdio.h>
int icp_thread_set_affinity(int core_id)
{
    int nb_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id < 0 || nb_cores <= core_id) {
        return (EINVAL);
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset));
}

int icp_thread_set_affinity_mask(const icp_cpuset_t cpuset)
{
    return (pthread_setaffinity_np(pthread_self(), icp_cpuset_get_native_size(cpuset), (cpu_set_t *)icp_cpuset_get_native_ptr(cpuset)));
}

int icp_thread_get_affinity_mask(icp_cpuset_t cpuset)
{
    return pthread_getaffinity_np(pthread_self(), icp_cpuset_get_native_size(cpuset), (cpu_set_t *)icp_cpuset_get_native_ptr(cpuset));
}

