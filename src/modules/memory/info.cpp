#include "memory/info.hpp"

#include <sys/sysinfo.h>

#include "core/op_core.h"

namespace openperf::memory::info {

MemoryInfoResultPointer memory_info::get() const
{
    struct sysinfo info;
    memset(&info, 0, sizeof(info));

    auto error = sysinfo(&info);
    if (error == 0) {
        auto ptr = MemoryInfoResultPointer(new MemoryInfoResult);
        ptr->setFreeMemory(info.freeram * info.mem_unit);
        ptr->setTotalMemory(info.totalram * info.mem_unit);
        return ptr;
    }

    OP_LOG(OP_LOG_ERROR,
           "Could not get system information. Error: %s",
           strerror(errno));

    return nullptr;
}

} // namespace openperf::memory::info
