#include "cpu/cpu_stats.hpp"

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

std::chrono::nanoseconds cpu_stats_get_steal_time()
{
    return 0ns;
}

} // namespace openperf::cpu::internal
