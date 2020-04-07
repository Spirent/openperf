#include "generator_result.hpp"

namespace openperf::block::model {

std::string block_generator_result::get_id() const { return id; }

void block_generator_result::set_id(std::string_view value) { id = value; }

bool block_generator_result::is_active() const { return active; }

void block_generator_result::set_active(bool value) { active = value; }

time_point block_generator_result::get_timestamp() const { return timestamp; }

void block_generator_result::set_timestamp(const time_point& value)
{
    timestamp = value;
}

block_generator_statistics block_generator_result::get_read_stats() const
{
    return read_stats;
}

void block_generator_result::set_read_stats(
    const block_generator_statistics& value)
{
    read_stats = value;
}

block_generator_statistics block_generator_result::get_write_stats() const
{
    return write_stats;
}

void block_generator_result::set_write_stats(
    const block_generator_statistics& value)
{
    write_stats = value;
}

} // namespace openperf::block::model