#include "generator_result.hpp"

namespace openperf::block::model {

std::string block_generator_result::get_id() const { return m_id; }

void block_generator_result::set_id(std::string_view value) { m_id = value; }

bool block_generator_result::is_active() const { return m_active; }

void block_generator_result::set_active(bool value) { m_active = value; }

time_point block_generator_result::get_timestamp() const { return m_timestamp; }

void block_generator_result::set_timestamp(const time_point& value)
{
    m_timestamp = value;
}

block_generator_statistics block_generator_result::get_read_stats() const
{
    return m_read_stats;
}

void block_generator_result::set_read_stats(
    const block_generator_statistics& value)
{
    m_read_stats = value;
}

block_generator_statistics block_generator_result::get_write_stats() const
{
    return m_write_stats;
}

void block_generator_result::set_write_stats(
    const block_generator_statistics& value)
{
    m_write_stats = value;
}

} // namespace openperf::block::model