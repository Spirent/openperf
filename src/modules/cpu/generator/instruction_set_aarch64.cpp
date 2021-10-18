#include "cpu/generator/instruction_set.hpp"

namespace openperf::cpu::instruction_set {

bool available(type t)
{
    switch (t) {
    case type::SCALAR:
    case type::AUTO:
    case type::NEON:
        return (true);
    default:
        return (false);
    }
}

/*
 * Since we only have one ISA available, we internally use the
 * AUTO mode, but present that as NEON to clients.
 */

std::string_view to_string(type t)
{
    if (t == type::AUTO) { t = type::NEON; }

    return (detail::to_string(t));
}

type to_type(std::string_view value)
{
    if (value == to_string(type::AUTO)) { value = to_string(type::NEON); }

    return (detail::to_type(value));
}

} // namespace openperf::cpu::instruction_set
