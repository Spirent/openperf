#include "generator.hpp"

namespace openperf::block::model {

std::string block_generator::get_id() const { return id; }
void block_generator::set_id(std::string_view value) { id = value; }

block_generator_config block_generator::get_config() const { return config; }
void block_generator::set_config(const block_generator_config& value)
{
    config = value;
}

std::string block_generator::get_resource_id() const { return resource_id; }
void block_generator::set_resource_id(std::string_view value)
{
    resource_id = value;
}

bool block_generator::is_running() const { return running; }
void block_generator::set_running(const bool value) { running = value; }

} // namespace openperf::block::model