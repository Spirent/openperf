#include "packet/generator/traffic/header/expand.hpp"
#include "packet/generator/traffic/header/expand_impl/expand.tcc"

namespace openperf::packet::generator::traffic::header {

container expand(const ethernet_config& config)
{
    return (detail::expand(config));
}

} // namespace openperf::packet::generator::traffic::header
