#include <memory>

#include "packetio/generic_stack.h"
#include "packetio/stack/dpdk/lwip.h"

namespace icp {
namespace packetio {
namespace stack {

std::unique_ptr<generic_stack> make(driver::generic_driver& driver)
{
    return std::make_unique<generic_stack>(
        generic_stack(icp::packetio::dpdk::lwip(driver)));
}

}
}
}
