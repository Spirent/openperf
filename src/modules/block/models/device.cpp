#include "device.hpp"

namespace openperf::block::model {

std::string device::get_id() const { return id; }

void device::set_id(std::string_view value) { id = value; }

std::string device::get_path() const { return path; }

void device::set_path(std::string_view value) { path = value; }

uint64_t device::get_size() const { return size; }

void device::set_size(const uint64_t value) { size = value; }

std::string device::get_info() const { return info; }

void device::set_info(std::string_view value) { info = value; }

bool device::is_usable() const { return usable; }

void device::set_usable(const bool value) { usable = value; }

} // namespace openperf::block::model