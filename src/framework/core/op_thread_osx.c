#include <pthread.h>

#include "core/op_thread.h"

int op_thread_setname(const char* name) { return pthread_setname_np(name); }

int op_thread_getname(pthread_t tid, char* name)
{
    return (pthread_getname_np(tid, name, OP_THREAD_NAME_MAX_LENGTH));
}

int op_thread_set_affinity(int core_id __attribute__((unused))) { return (-1); }

int op_thread_set_affinity_mask(const op_cpu_mask_t* cpu_mask
                                __attribute__((unused)))
{
    return (-1);
}

int op_thread_get_affinity_mask(op_cpu_mask_t* cpu_mask __attribute__((unused)))
{
    return (-1);
}
