#include "cpu_info.hpp"

namespace openperf::cpu::model {

int32_t cpu_info::get_cores() const
{
    return m_cores;
}

void cpu_info::set_cores(int32_t value)
{
    m_cores = value;
}

int64_t cpu_info::get_cache_line_size() const
{
    return m_cache_line_size;
}

void cpu_info::set_cache_line_size(int64_t value)
{
    m_cache_line_size = value;
}

std::string cpu_info::get_architecture() const
{
    return m_architecture;
}

void cpu_info::set_architecture(std::string_view value)
{
    m_architecture = value;
}

}