#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/resource.h>

#include "cpu/generator/system_stats.hpp"

namespace openperf::cpu::generator {

/* All of these are present as of 2.6.11 */
struct linux_proc_stats
{
    std::string cpu;
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
};

std::chrono::microseconds get_core_steal_time(uint8_t core_id)
{
    using namespace std::chrono_literals;

    auto ticks_per_sec = sysconf(_SC_CLK_TCK);
    if (ticks_per_sec == -1) { return (0ms); }

    auto procstat = std::ifstream("/proc/stat");
    auto line = std::string{};
    auto stats = linux_proc_stats{};

    while (std::getline(procstat, line)) {
        auto input = std::istringstream(line);
        if (!(input >> stats.cpu >> stats.user >> stats.nice >> stats.system
              >> stats.idle >> stats.iowait >> stats.irq >> stats.softirq
              >> stats.steal)) {
            break; /* error processing input data */
        }

        if (stats.cpu == "cpu" + std::to_string(core_id)) {
            auto us =
                stats.steal
                * std::chrono::duration_cast<std::chrono::microseconds>(1s)
                      .count()
                / ticks_per_sec;
            return (std::chrono::microseconds(us));
        }
    }

    return (0ms);
}

std::pair<timeval, timeval> get_thread_cpu_usage()
{
    auto usage = rusage{};
    getrusage(RUSAGE_THREAD, &usage);
    return {usage.ru_stime, usage.ru_utime};
}

} // namespace openperf::cpu::generator
