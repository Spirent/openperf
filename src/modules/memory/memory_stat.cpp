#include "memory_stat.hpp"

#include <set>

namespace openperf::memory::internal {

task_memory_stat& task_memory_stat::operator+=(const task_memory_stat& st)
{
    bytes += st.bytes;
    bytes_target += st.bytes_target;
    errors += st.errors;
    operations += st.operations;
    operations_target += st.operations_target;
    run_time += st.run_time;
    sleep_time += st.sleep_time;
    timestamp = std::max(timestamp, st.timestamp);

    latency_min = [&]() -> optional_time_t {
        if (latency_min.has_value() && st.latency_min.has_value())
            return std::min(latency_min.value(), st.latency_min.value());

        return latency_min.has_value() ? latency_min : st.latency_min;
    }();

    latency_max = [&]() -> optional_time_t {
        if (latency_max.has_value() && st.latency_max.has_value())
            return std::max(latency_max.value(), st.latency_max.value());

        return latency_max.has_value() ? latency_max : st.latency_max;
    }();

    return *this;
}

task_memory_stat task_memory_stat::operator+(const task_memory_stat& st)
{
    auto stat = *this;
    return stat += st;
}

task_memory_stat::timestamp_t memory_stat::timestamp() const
{
    return std::max(read.timestamp, write.timestamp);
}

} // namespace openperf::memory::internal
