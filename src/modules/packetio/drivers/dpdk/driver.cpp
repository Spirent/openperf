#include "packetio/generic_driver.h"
#include "packetio/drivers/dpdk/arg_parser.h"
#include "packetio/drivers/dpdk/eal.h"

#include <memory>

namespace icp {
namespace packetio {
namespace driver {

using namespace icp::packetio::dpdk;

std::unique_ptr<generic_driver> make(void* context)
{
    auto& parser = arg_parser::instance();
    if (parser.test_mode()) {
        return std::make_unique<generic_driver>(
            generic_driver(icp::packetio::dpdk::eal(context, parser.args(), parser.test_portpairs())));
    } else {
        return std::make_unique<generic_driver>(
            generic_driver(icp::packetio::dpdk::eal(context, parser.args())));
    }
}

}
}
}
