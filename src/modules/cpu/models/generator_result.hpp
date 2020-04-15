#ifndef _OP_CPU_GENERATOR_RESULT_MODEL_HPP_
#define _OP_CPU_GENERATOR_RESULT_MODEL_HPP_

#include <chrono>
#include <string>
#include <memory>
#include "timesync/chrono.hpp"

namespace openperf::cpu::model {

using time_point = std::chrono::time_point<timesync::chrono::realtime>;

struct cpu_generator_target_stats
{
    uint64_t cycles;
};

struct cpu_generator_core_stats
{
    uint64_t available;
    uint64_t utilization;
    uint64_t system;
    uint64_t user;
    uint64_t steal;
    uint64_t error;
    std::vector<cpu_generator_target_stats> targets;
};

struct cpu_generator_stats
{
    std::vector<cpu_generator_core_stats> cores;
};

class cpu_generator_result
{
public:
    cpu_generator_result() = default;
    cpu_generator_result(const cpu_generator_result&) = default;

    std::string get_id() const;
    void set_id(std::string_view value);

    std::string get_generator_id() const;
    void set_generator_id(std::string_view value);

    bool is_active() const;
    void set_active(bool value);

    time_point get_timestamp() const;
    void set_timestamp(const time_point& value);

    cpu_generator_stats get_stats() const;
    void set_stats(const cpu_generator_stats& value);

protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    cpu_generator_stats m_stats;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_GENERATOR_RESULT_MODEL_HPP_