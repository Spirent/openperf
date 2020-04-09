#include "generator.hpp"

namespace openperf::block::model {

std::string block_generator::get_id() const { return m_id; }
void block_generator::set_id(std::string_view value) { m_id = value; }

block_generator_config block_generator::get_config() const { return m_config; }
void block_generator::set_config(const block_generator_config& value)
{
    m_config = value;
}

std::string block_generator::get_resource_id() const { return m_resource_id; }
void block_generator::set_resource_id(std::string_view value)
{
    m_resource_id = value;
}

bool block_generator::is_running() const { return m_running; }
void block_generator::set_running(const bool value) { m_running = value; }

} // namespace openperf::block::model