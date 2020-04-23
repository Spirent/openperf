#include "file.hpp"

namespace openperf::block::model {

file::file(const file& f)
    : m_id(f.get_id())
    , m_size(f.get_size())
    , m_init_percent_complete(f.get_init_percent_complete())
    , m_path(f.get_path())
    , m_state(f.get_state())
{}

std::string file::get_id() const { return m_id; }
void file::set_id(std::string_view value) { m_id = value; }

uint64_t file::get_size() const { return m_size; }
void file::set_size(const uint64_t value) { m_size = value; }

int32_t file::get_init_percent_complete() const
{
    return m_init_percent_complete;
}
void file::set_init_percent_complete(const int32_t value)
{
    m_init_percent_complete = value;
}

std::string file::get_path() const { return m_path; }
void file::set_path(std::string_view value) { m_path = value; }

file_state file::get_state() const { return m_state; }

void file::set_state(const file_state& value) { m_state = value; }

} // namespace openperf::block::model