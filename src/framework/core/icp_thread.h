#ifndef _ICP_THREAD_H_
#define _ICP_THREAD_H_

#include <pthread.h>
#include "icp_cpuset.h"

#define ICP_THREAD_NAME_MAX_LENGTH 16

#ifdef __cplusplus
extern "C" {
#endif

int icp_thread_setname(const char *name);

int icp_thread_getname(pthread_t tid, char *name);

int icp_thread_set_affinity(int core_id);

int icp_thread_set_affinity_mask(icp_cpuset_t cpuset);

int icp_thread_get_affinity_mask(icp_cpuset_t cpuset);

#ifdef __cplusplus
}
#endif

#endif /* _ICP_THREAD_H_ */
