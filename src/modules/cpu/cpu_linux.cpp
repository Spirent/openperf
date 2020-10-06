#include "cpu.hpp"

#include <cassert>
#include <cstdio>
#include <stdexcept>

#include <unistd.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/utsname.h>

namespace openperf::cpu::internal {

struct linux_proc_stats
{
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
};

linux_proc_stats cpu_stats()
{
    FILE* procstat = nullptr;

    while (true) {
        procstat = fopen("/proc/stat", "r");
        if (!procstat) break;

        /* Read and throw away 'cpu ' */
        char tag[8];
        if (fscanf(procstat, "%s ", tag) == EOF) break;

        auto stats = linux_proc_stats{};
        /* Pull out the values of interest */
        if (fscanf(procstat, "%" PRIu64 " ", &stats.user) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.nice) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.system) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.idle) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.iowait) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.irq) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.softirq) == EOF) break;
        if (fscanf(procstat, "%" PRIu64 " ", &stats.steal) == EOF) break;

        fclose(procstat);
        procstat = nullptr;

        return stats;
    }

    if (procstat) fclose(procstat);

    throw std::runtime_error("Error fetch CPU stats");
}

std::chrono::nanoseconds cpu_steal_time()
{
    auto ticks_per_sec = sysconf(_SC_CLK_TCK);
    assert(ticks_per_sec);

    auto stats = cpu_stats();
    return std::chrono::nanoseconds(stats.steal * std::nano::den
                                    / ticks_per_sec);
}

utilization_time cpu_thread_time()
{
    auto ru = rusage{};
    getrusage(RUSAGE_THREAD, &ru);

    auto time_system = std::chrono::seconds(ru.ru_stime.tv_sec)
                       + std::chrono::microseconds(ru.ru_stime.tv_usec);

    auto time_user = std::chrono::seconds(ru.ru_utime.tv_sec)
                     + std::chrono::microseconds(ru.ru_utime.tv_usec);

    return utilization_time{.user = time_user,
                            .system = time_system,
                            .steal = 0ns,
                            .utilization = time_user + time_system};
}

utilization_time cpu_process_time()
{
    auto ru = rusage{};
    getrusage(RUSAGE_SELF, &ru);

    auto time_system = std::chrono::seconds(ru.ru_stime.tv_sec)
                       + std::chrono::microseconds(ru.ru_stime.tv_usec);

    auto time_user = std::chrono::seconds(ru.ru_utime.tv_sec)
                     + std::chrono::microseconds(ru.ru_utime.tv_usec);

    return utilization_time{.user = time_user,
                            .system = time_system,
                            .steal = cpu_steal_time(),
                            .utilization = time_user + time_system};
}

uint16_t cpu_cache_line_size() { return sysconf(_SC_LEVEL1_DCACHE_LINESIZE); }

std::string cpu_architecture()
{
    auto info = utsname{};
    uname(&info);
    return info.machine;
}

} // namespace openperf::cpu::internal
