#include "packetio/generic_driver.hpp"
#include "packetio/drivers/dpdk/driver.tcc"
#include "packetio/drivers/dpdk/dummy_driver.hpp"
#include "packetio/drivers/dpdk/primary/arg_parser.hpp"
#include "packetio/drivers/dpdk/primary/eal_process.hpp"

namespace openperf::packetio {
template class dpdk::driver<dpdk::primary::test_port_process>;
template class dpdk::driver<dpdk::primary::live_port_process>;
} // namespace openperf::packetio

namespace openperf::packetio::driver {

std::unique_ptr<generic_driver> make()
{
    /*
     * Our driver depends on various command line options.
     * Check the options and respond appropriately.
     */
    if (dpdk::config::dpdk_disabled()) {
        return (std::make_unique<generic_driver>(dpdk::dummy_driver()));
    }

    if (dpdk::config::dpdk_test_mode()) {
        return (std::make_unique<generic_driver>(
            dpdk::driver<dpdk::primary::test_port_process>()));
    }

    return (std::make_unique<generic_driver>(
        dpdk::driver<dpdk::primary::live_port_process>()));
}

} // namespace openperf::packetio::driver
