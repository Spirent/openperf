#include "instruction_set.hpp"

namespace regurgitate::instruction_set {

bool available(type t)
{
    switch (t) {
    case type::AUTO:
    case type::NEON:
        return (true);
    default:
        return (false);
    }
}

} // namespace regurgitate::instruction_set
