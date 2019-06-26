#include <memory>

#include "packetio/generic_stack.h"
#include "packetio/stack/dpdk/lwip.h"

namespace icp::packetio::stack {

std::unique_ptr<generic_stack> make(driver::generic_driver& driver,
                                    workers::generic_workers& workers)
{
    return std::make_unique<generic_stack>(icp::packetio::dpdk::lwip(driver, workers));
}

}
