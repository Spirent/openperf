#include "packetio/generic_driver.h"
#include "packetio/drivers/dpdk/arg_parser.h"
#include "packetio/drivers/dpdk/eal.h"

#include <memory>

namespace icp::packetio::driver {

std::unique_ptr<generic_driver> make()
{
    namespace cfg = icp::packetio::dpdk::config;

    return (std::make_unique<generic_driver>(
                cfg::dpdk_test_mode()
                ? icp::packetio::dpdk::eal::test_environment(cfg::dpdk_args(),
                                                             cfg::dpdk_id_map(),
                                                             cfg::dpdk_test_portpairs())
                : icp::packetio::dpdk::eal::real_environment(cfg::dpdk_args(),
                                                             cfg::dpdk_id_map())));
}

}
