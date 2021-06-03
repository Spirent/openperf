#include "packetio/generic_driver.hpp"
#include "packetio/drivers/dpdk/driver.tcc"
#include "packetio/drivers/dpdk/dummy_driver.hpp"
#include "packetio/drivers/dpdk/secondary/eal_process.hpp"

namespace openperf::packetio {
template class dpdk::driver<dpdk::secondary::eal_process>;
} // namespace openperf::packetio

namespace openperf::packetio::driver {

std::unique_ptr<generic_driver> make()
{
    if (dpdk::config::dpdk_disabled()) {
        return (std::make_unique<generic_driver>(dpdk::dummy_driver()));
    }

    return (std::make_unique<generic_driver>(
        dpdk::driver<dpdk::secondary::eal_process>()));
}

} // namespace openperf::packetio::driver
