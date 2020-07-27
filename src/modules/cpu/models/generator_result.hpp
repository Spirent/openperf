#ifndef _OP_CPU_GENERATOR_RESULT_MODEL_HPP_
#define _OP_CPU_GENERATOR_RESULT_MODEL_HPP_

#include <string_view>

#include "modules/timesync/chrono.hpp"
#include "cpu/cpu_stat.hpp"

namespace openperf::cpu::model {

class generator_result
{
private:
    using time_point = timesync::chrono::realtime::time_point;

public:
    generator_result() = default;
    generator_result(const generator_result&) = default;

    std::string id() const { return m_id; }
    void id(std::string_view id) { m_id = id; }

    std::string generator_id() const { return m_generator_id; }
    void generator_id(std::string_view id) { m_generator_id = id; }

    bool active() const { return m_active; }
    void active(bool active) { m_active = active; }

    time_point timestamp() const { return m_timestamp; }
    void timestamp(const time_point& time) { m_timestamp = time; }

    cpu_stat stats() const { return m_stats; }
    void stats(const cpu_stat& stats) { m_stats = stats; }

protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    cpu_stat m_stats;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_GENERATOR_RESULT_MODEL_HPP_