#ifndef _OP_THREAD_H_
#define _OP_THREAD_H_

#include <pthread.h>
#include "op_cpuset.h"

#define OP_THREAD_NAME_MAX_LENGTH 16

#ifdef __cplusplus
extern "C" {
#endif

int op_thread_setname(const char* name);

int op_thread_getname(pthread_t tid, char* name);

int op_thread_set_affinity(int core_id);

int op_thread_set_affinity_mask(op_cpuset_t cpuset);

int op_thread_get_affinity_mask(op_cpuset_t cpuset);

#ifdef __cplusplus
}
#endif

#endif /* _OP_THREAD_H_ */
