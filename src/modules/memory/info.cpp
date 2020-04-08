#include <sys/sysinfo.h>

#include "memory/info.hpp"
#include "core/op_core.h"

namespace openperf::memory {

memory_info::info_t memory_info::get()
{
    struct sysinfo info;
    memset(&info, 0, sizeof(info));

    if (sysinfo(&info) == 0) {
        return info_t{
            .total = info.totalram * info.mem_unit,
            .free = info.freeram * info.mem_unit,
        };
    }

    OP_LOG(OP_LOG_ERROR,
           "Could not get system information. Error: %s",
           strerror(errno));

    throw std::runtime_error("Could not get system information. Error: "
                             + std::string(strerror(errno)));
}

} // namespace openperf::memory
