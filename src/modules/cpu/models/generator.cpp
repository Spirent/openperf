#include "generator.hpp"

namespace openperf::cpu::model {

std::string cpu_generator::get_id() const { return m_id; }
void cpu_generator::set_id(std::string_view value) { m_id = value; }

cpu_generator_config cpu_generator::get_config() const { return m_config; }
void cpu_generator::set_config(const cpu_generator_config& value)
{
    m_config = value;
}

bool cpu_generator::is_running() const { return m_running; }
void cpu_generator::set_running(const bool value) { m_running = value; }

} // namespace openperf::cpu::model