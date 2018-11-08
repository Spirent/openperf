#include "packetio/generic_driver.h"
#include "drivers/dpdk/arg_parser.h"
#include "drivers/dpdk/eal.h"

#include <memory>

namespace icp {
namespace packetio {
namespace driver {

using namespace icp::packetio::dpdk;

std::unique_ptr<generic_driver> make(void* context)
{
    auto& parser = arg_parser::instance();
    return std::make_unique<generic_driver>(
        generic_driver(icp::packetio::dpdk::eal(context, parser.args())));
}

}
}
}
