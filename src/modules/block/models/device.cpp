#include "device.hpp"

namespace openperf::block::model {

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

} // namespace openperf::block::model