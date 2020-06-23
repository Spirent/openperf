#include "device.hpp"

namespace openperf::block::model {

device::device(const device& f)
    : m_id(f.get_id())
    , m_path(f.get_path())
    , m_size(f.get_size())
    , m_info(f.get_info())
    , m_usable(f.is_usable())
    , m_init_percent_complete(f.get_init_percent_complete())
    , m_state(f.get_state())
{}

std::string device::get_id() const { return m_id; }

void device::set_id(std::string_view value) { m_id = value; }

std::string device::get_path() const { return m_path; }

void device::set_path(std::string_view value) { m_path = value; }

uint64_t device::get_size() const { return m_size; }

void device::set_size(const uint64_t value) { m_size = value; }

std::string device::get_info() const { return m_info; }

void device::set_info(std::string_view value) { m_info = value; }

bool device::is_usable() const { return m_usable; }

void device::set_usable(const bool value) { m_usable = value; }

int32_t device::get_init_percent_complete() const
{
    return m_init_percent_complete;
}
void device::set_init_percent_complete(const int32_t value)
{
    m_init_percent_complete = value;
}

device::state device::get_state() const { return m_state; }

void device::set_state(const device::state& value) { m_state = value; }

} // namespace openperf::block::model