#include "config/op_config_file.hpp"
#include "network/arg_parser.hpp"
#include "network/drivers/dpdk.hpp"
#include "network/drivers/kernel.hpp"

namespace openperf::network::config {

std::optional<core::cpuset> core_mask()
{
    if (const auto mask = openperf::config::file::op_config_get_param<
            OP_OPTION_TYPE_CPUSET_STRING>(op_network_mask)) {
        return (core::cpuset_online() & core::cpuset(mask.value()));
    }

    return (std::nullopt);
}

std::shared_ptr<internal::drivers::driver> driver()
{
    using namespace internal::drivers;

    auto driver =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
            op_network_driver);
    if (!driver || driver.value().compare(kernel::key) == 0) {
        return (driver::instance<kernel>());
    } else if (driver.value().compare(dpdk::key) == 0) {
        return (driver::instance<dpdk>());
    }

    throw std::runtime_error("Network driver " + driver.value()
                             + " is unsupported");
}

/*
 * XXX: value is used on a hot path, so we use a static here to store
 * any user specified value for repeated calls.
 */
std::chrono::microseconds operation_timeout()
{
    using namespace std::chrono_literals;
    constexpr auto default_timeout = 1s;

    static const auto timeout =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
            op_network_op_timeout);

    return (timeout ? std::chrono::microseconds(timeout.value())
                    : default_timeout);
}

} // namespace openperf::network::config
