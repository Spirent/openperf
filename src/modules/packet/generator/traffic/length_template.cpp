#include "packet/generator/traffic/length_template.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packet::generator::traffic {

length_container get_lengths(const length_config& config)
{
    return (std::visit(
        utils::overloaded_visitor(
            [](const length_fixed_config& fixed) {
                return (length_container{fixed.length});
            },
            [](const length_list_config& list) { return (list.items); },
            [](const length_sequence_config& seq) {
                return (ranges::to<length_container>(modifier::to_range(seq)));
            }),
        config));
}

length_container get_lengths(length_config&& config)
{
    return (get_lengths(config));
}

length_template::length_template(length_container&& lengths)
    : m_lengths(std::move(lengths))
{}

uint16_t length_template::max_packet_length() const
{
    return (*(std::max_element(std::begin(m_lengths), std::end(m_lengths))));
}

size_t length_template::size() const { return (m_lengths.size()); }

length_template::view_type length_template::operator[](size_t idx) const
{
    return (m_lengths[idx]);
}

length_template::iterator length_template::begin() { return (iterator(*this)); }

length_template::const_iterator length_template::begin() const
{
    return (iterator(*this));
}

length_template::iterator length_template::end()
{
    return (iterator(*this, size()));
}

length_template::const_iterator length_template::end() const
{
    return (iterator(*this, size()));
}

} // namespace openperf::packet::generator::traffic
