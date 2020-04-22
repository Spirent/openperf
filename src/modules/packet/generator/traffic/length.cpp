#include "packet/generator/traffic/length.hpp"
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

} // namespace openperf::packet::generator::traffic
