#include <memory>

#include "packet/stack/generic_stack.hpp"
#include "packet/stack/dpdk/lwip.hpp"

namespace openperf::packet::stack {

std::unique_ptr<generic_stack> make(void* context)
{
    return (std::make_unique<generic_stack>(
        openperf::packet::stack::dpdk::lwip(context)));
}

} // namespace openperf::packet::stack
