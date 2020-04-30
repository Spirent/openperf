#include "cpu_info.hpp"

namespace openperf::cpu::info {

int32_t cores_count()
{
    return sysconf (_SC_NPROCESSORS_CONF);
}

int64_t cache_line_size()
{
    return sysconf (_SC_LEVEL1_DCACHE_LINESIZE);
}

std::string architecture()
{
    return ARCH;
}

} // namespace openperf::cpu::info
