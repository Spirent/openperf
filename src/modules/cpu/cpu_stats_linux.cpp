#include "cpu/cpu_stats.hpp"

#include <cassert>
#include <cstdio>
#include <cinttypes>
#include <stdexcept>

#include <unistd.h>

#include <sys/resource.h>
#include <sys/time.h>

namespace openperf::cpu::internal {

struct linux_proc_stats {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
};

linux_proc_stats get_cpu_stats()
{
    FILE *procstat = NULL;

    while (true) {
        procstat = fopen("/proc/stat", "r");
        if (!procstat)
            break;

        /* Read and throw away 'cpu ' */
        char tag[8];
        if (fscanf(procstat, "%s ", tag) == EOF)
            break;

        auto stats = linux_proc_stats{};
        /* Pull out the values of interest */
        if (fscanf(procstat, "%" PRIu64 " ", &stats.user) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.nice) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.system) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.idle) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.iowait) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.irq) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.softirq) == EOF)
            break;

        if (fscanf(procstat, "%" PRIu64 " ", &stats.steal) == EOF)
            break;

        fclose(procstat);
        procstat = NULL;

        return stats;
    }

    if (procstat)
        fclose(procstat);

    throw std::runtime_error("Error fetch CPU stats");
}

std::chrono::nanoseconds get_steal_time()
{
    auto ticks_per_sec = sysconf(_SC_CLK_TCK);
    assert(ticks_per_sec);

    auto stats = get_cpu_stats();
    return std::chrono::nanoseconds(
        stats.steal * std::nano::den / ticks_per_sec);
}

utilization_time get_thread_time()
{
    auto ru = rusage{};
    getrusage(RUSAGE_THREAD, &ru);

    auto time_system = std::chrono::seconds(ru.ru_stime.tv_sec)
        + std::chrono::microseconds(ru.ru_stime.tv_usec);

    auto time_user = std::chrono::seconds(ru.ru_utime.tv_sec)
        + std::chrono::microseconds(ru.ru_utime.tv_usec);

    return utilization_time{
        .user = time_user,
        .system = time_system,
        .steal = 0ns,
        .utilization = time_user + time_system
    };
}

} // namespace openperf::cpu::internal
