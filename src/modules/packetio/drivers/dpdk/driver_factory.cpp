#include "packetio/generic_driver.h"
#include "packetio/drivers/dpdk/arg_parser.h"
#include "packetio/drivers/dpdk/eal.h"

#include <memory>

namespace icp::packetio::driver {

std::unique_ptr<generic_driver> make()
{
    auto& parser = dpdk::arg_parser::instance();
    return (std::make_unique<generic_driver>(
                parser.test_mode()
                ? icp::packetio::dpdk::eal::test_environment(parser.args(),
                                                             parser.id_map(),
                                                             parser.test_portpairs())
                : icp::packetio::dpdk::eal::real_environment(parser.args(),
                                                             parser.id_map())));
}

}
