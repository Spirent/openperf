#include "instruction_set.h"

namespace pga::instruction_set {

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

} // namespace pga::instruction_set
