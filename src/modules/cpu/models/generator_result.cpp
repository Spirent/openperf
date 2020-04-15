#include "generator_result.hpp"

namespace openperf::cpu::model {

std::string cpu_generator_result::get_id() const { return m_id; }

void cpu_generator_result::set_id(std::string_view value) { m_id = value; }

std::string cpu_generator_result::get_generator_id() const { return m_generator_id; }

void cpu_generator_result::set_generator_id(std::string_view value) { m_generator_id = value; }

bool cpu_generator_result::is_active() const { return m_active; }

void cpu_generator_result::set_active(bool value) { m_active = value; }

time_point cpu_generator_result::get_timestamp() const { return m_timestamp; }

void cpu_generator_result::set_timestamp(const time_point& value)
{
    m_timestamp = value;
}

cpu_generator_stats cpu_generator_result::get_stats() const
{
    return m_stats;
}

void cpu_generator_result::set_stats(const cpu_generator_stats& value)
{
    m_stats = value;
}

} // namespace openperf::cpu::model