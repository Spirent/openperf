#ifndef _OP_CPU_GENERATOR_RESULT_MODEL_HPP_
#define _OP_CPU_GENERATOR_RESULT_MODEL_HPP_

#include <chrono>
#include <string>
#include <memory>
#include "timesync/chrono.hpp"

namespace openperf::cpu::model {

using time_point = std::chrono::time_point<timesync::chrono::realtime>;

struct generator_target_stats
{
    uint64_t cycles;
};

struct generator_core_stats
{
    uint64_t available;
    uint64_t utilization;
    std::chrono::nanoseconds system;
    std::chrono::nanoseconds user;
    std::chrono::nanoseconds steal;
    //uint64_t system;
    //uint64_t user;
    //uint64_t steal;
    uint64_t error;
    std::vector<generator_target_stats> targets;
};

struct generator_stats
{
    std::vector<generator_core_stats> cores;
};

class generator_result
{
public:
    generator_result() = default;
    generator_result(const generator_result&) = default;

    inline std::string id() const { return m_id; }
    inline void id(std::string_view id) { m_id = id; }

    inline std::string generator_id() const { return m_generator_id; }
    inline void generator_id(std::string_view id) { m_generator_id = id; }

    inline bool active() const { return m_active; }
    inline void active(bool active) { m_active = active; }

    inline time_point timestamp() const { return m_timestamp; }
    inline void timestamp(const time_point& time) { m_timestamp = time; }

    inline generator_stats stats() const { return m_stats; }
    inline void stats(const generator_stats& stats) { m_stats = stats; }

protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    generator_stats m_stats;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_GENERATOR_RESULT_MODEL_HPP_