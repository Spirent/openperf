#include "memory/info.hpp"

#include <sys/sysinfo.h>

#include "core/op_core.h"

namespace openperf::memory {

model::MemoryInfoResult memory_info::get()
{
    struct sysinfo info;
    memset(&info, 0, sizeof(info));

    auto error = sysinfo(&info);
    if (error == 0) {
        model::MemoryInfoResult result;
        result.setFreeMemory(info.freeram * info.mem_unit);
        result.setTotalMemory(info.totalram * info.mem_unit);
        return result;
    }

    OP_LOG(OP_LOG_ERROR,
           "Could not get system information. Error: %s",
           strerror(errno));

    throw std::runtime_error(
       "Could not get system information. Error: " 
       + std::string(strerror(errno)));
}

} // namespace openperf::memory::info
