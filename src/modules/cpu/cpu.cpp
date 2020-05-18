#include "cpu.hpp"

#ifndef ARCH
#define ARCH "unknown"
#endif

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

std::chrono::nanoseconds cpu_steal_time()
{
    return 0ns;
}

utilization_time cpu_thread_time()
{
    return utilization_time{};
}

int32_t cpu_cores_count()
{
    return 0;
}

int64_t cpu_cache_line_size()
{
    return 0;
}

std::string cpu_architecture()
{
    return ARCH;
}

} // namespace openperf::cpu::internal
