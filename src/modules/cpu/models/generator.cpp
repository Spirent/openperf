#include "generator.hpp"

namespace openperf::cpu::model {

std::string cpu_generator::get_id() const { return id; }
void cpu_generator::set_id(std::string_view value) { id = value; }

cpu_generator_config cpu_generator::get_config() const { return config; }
void cpu_generator::set_config(const cpu_generator_config& value)
{
    config = value;
}

bool cpu_generator::is_running() const { return running; }
void cpu_generator::set_running(const bool value) { running = value; }

} // namespace openperf::cpu::model