#include <memory>

#include "packetio/generic_stack.hpp"
#include "packetio/stack/dpdk/lwip.hpp"

namespace openperf::packetio::stack {

std::unique_ptr<generic_stack> make(driver::generic_driver& driver,
                                    workers::generic_workers& workers)
{
    return std::make_unique<generic_stack>(
        openperf::packetio::dpdk::lwip(driver, workers));
}

} // namespace openperf::packetio::stack
