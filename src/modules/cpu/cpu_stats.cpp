#include "cpu/cpu_stats.hpp"

namespace openperf::cpu::internal {

using namespace std::chrono_literals;

std::chrono::nanoseconds get_steal_time()
{
    return 0ns;
}

utilization_time get_thread_time()
{
    return utilization_time{};
}

} // namespace openperf::cpu::internal
