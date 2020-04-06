#include "device.hpp"

namespace openperf::block::model {

std::string device::get_id() const
{
    return id;
}

void device::set_id(const std::string& value)
{
    id = value;
}

std::string device::get_path() const
{
    return path;
}

void device::set_path(const std::string& value)
{
    path = value;
}

uint64_t device::get_size() const
{
    return size;
}

void device::set_size(const uint64_t value)
{
    size = value;
}

std::string device::get_info() const
{
    return info;
}

void device::set_info(const std::string& value)
{
    info = value;
}

bool device::is_usable() const
{
    return usable;
}

void device::set_usable(const bool value)
{
    usable = value;
}

} // openperf::block::model