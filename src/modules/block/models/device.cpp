#include "device.hpp"

namespace openperf::block::model {

device::device(const device& f)
    : m_id(f.m_id)
    , m_path(f.m_path)
    , m_size(f.m_size)
    , m_info(f.m_info)
    , m_usable(f.m_usable)
    , m_init_percent_complete(f.m_init_percent_complete.load())
    , m_state(f.m_state.load())
{}

std::string device::get_id() const { return m_id; }

void device::set_id(std::string_view value) { m_id = value; }

std::string device::get_path() const { return m_path; }

void device::set_path(std::string_view value) { m_path = value; }

uint64_t device::get_size() const { return m_size; }

void device::set_size(uint64_t value) { m_size = value; }

std::string device::get_info() const { return m_info; }

void device::set_info(std::string_view value) { m_info = value; }

bool device::is_usable() const { return m_usable; }

void device::set_usable(bool value) { m_usable = value; }

int32_t device::get_init_percent_complete() const
{
    return m_init_percent_complete;
}
void device::set_init_percent_complete(int32_t value)
{
    m_init_percent_complete = value;
}

device::state device::get_state() const { return m_state; }

void device::set_state(device::state value) { m_state = value; }

} // namespace openperf::block::model