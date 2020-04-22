#include "packet/generator/traffic/packet_template.hpp"

namespace openperf::packet::generator::traffic {

packet_template::packet_template(const header::config_container& configs,
                                 header::modifier_mux mux,
                                 const std::vector<uint16_t> lengths)
    : m_headers(header::make_headers(configs, mux))
    , m_lengths(lengths)
    , m_key(header::get_config_key(configs))
{}

uint16_t packet_template::max_packet_length() const
{
    return (*(std::max_element(std::begin(m_lengths), std::end(m_lengths))));
}

size_t packet_template::size() const { return (m_headers.size()); }

packet_template::view_type packet_template::operator[](size_t idx) const
{
    assert(idx < size());

    auto&& [ptr, length] = m_headers[idx];
    return {{m_key, ptr, length}, m_lengths[idx % m_lengths.size()]};
}

bool packet_template::operator==(const packet_template& other) const
{
    return (m_lengths == other.m_lengths && m_key == other.m_key
            && m_headers == other.m_headers);
}

packet_template::iterator packet_template::begin() { return (iterator(*this)); }

packet_template::const_iterator packet_template::begin() const
{
    return (iterator(*this));
}

packet_template::iterator packet_template::end()
{
    return (iterator(*this, size()));
}

packet_template::const_iterator packet_template::end() const
{
    return (iterator(*this, size()));
}

} // namespace openperf::packet::generator::traffic
