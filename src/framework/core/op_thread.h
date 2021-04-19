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

#ifdef __cplusplus

#include <stdexcept>
#include <vector>

inline std::vector<uint16_t> op_get_cpu_online()
{
    auto current_cpuset = op_cpuset_create();
    auto test_cpuset = op_cpuset_all();

    try {
        if (op_thread_get_affinity_mask(current_cpuset))
            throw std::runtime_error("Failed to get affinity");

        if (op_thread_set_affinity_mask(test_cpuset))
            throw std::runtime_error("Failed to set affinity");

        if (op_thread_get_affinity_mask(test_cpuset))
            throw std::runtime_error("Failed to get affinity");

        if (op_thread_set_affinity_mask(current_cpuset))
            throw std::runtime_error("Failed to set affinity");
    } catch (const std::runtime_error&) {
        op_cpuset_delete(current_cpuset);
        op_cpuset_delete(test_cpuset);
        throw;
    }

    std::vector<uint16_t> core_list;
    for (uint16_t core = 0; core < op_cpuset_size(test_cpuset); core++) {
        if (op_cpuset_get(test_cpuset, core)) core_list.push_back(core);
    }

    op_cpuset_delete(current_cpuset);
    op_cpuset_delete(test_cpuset);
    return core_list;
}

inline int op_thread_set_relative_affinity_mask(unsigned cpuset_hex)
{
    auto cpuset = op_cpuset_create();
    auto available_core_list = op_get_cpu_online();

    for (size_t i = 0; i < available_core_list.size(); i++) {
        if (cpuset_hex & (1 << i)) {
            op_cpuset_set(cpuset, available_core_list.at(i), true);
        }
    }

    auto r = op_thread_set_affinity_mask(cpuset);
    op_cpuset_delete(cpuset);
    return r;
}

#endif

#endif /* _OP_THREAD_H_ */
