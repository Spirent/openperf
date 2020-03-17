#include "file.hpp"

namespace openperf::block::model {

std::string file::get_id() const
{
    return id;
}
void file::set_id(const std::string& value)
{
    id = value;
}

int64_t file::get_size() const
{
    return size;
}
void file::set_size(const int64_t value)
{
    size = value;
}

int32_t file::get_init_percent_complete() const
{
    return init_percent_complete;
}
void file::set_init_percent_complete(const int32_t value)
{
    init_percent_complete = value;
}

std::string file::get_path() const
{
    return path;
}
void file::set_path(const std::string& value)
{
    path = value;
}

std::string file::get_state() const
{
    return state;
}
void file::set_state(const std::string& value)
{
    state = value;
}

} // openperf::block::model