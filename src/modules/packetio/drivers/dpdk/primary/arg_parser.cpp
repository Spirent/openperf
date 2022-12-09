#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/primary/arg_parser.hpp"

namespace openperf::packetio::dpdk::config {

using namespace openperf::config;

inline constexpr std::string_view no_pci_arg = "--no-pci";

std::vector<std::string> dpdk_args()
{
    auto args = common_dpdk_args();

    if (dpdk_test_mode() && !contains(args, no_pci_arg)) {
        args.emplace_back(no_pci_arg);
    }

    return (args);
}

int dpdk_test_portpairs()
{
    static const auto port_pairs =
        config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
            op_packetio_dpdk_test_portpairs)
            .value_or(dpdk_test_mode() ? 1 : 0);

    return (port_pairs);
}

bool dpdk_test_mode()
{
    static const auto test_mode =
        config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
            op_packetio_dpdk_test_mode)
            .value_or(false);

    return (test_mode);
}

bool dpdk_disable_lro()
{
    static const auto no_lro =
        config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
            op_packetio_dpdk_no_lro)
            .value_or(false);

    return (no_lro);
}

bool dpdk_disable_rx_irq()
{
    static const auto no_rx_irqs =
        config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
            op_packetio_dpdk_no_rx_irqs)
            .value_or(false);

    return (no_rx_irqs);
}

} // namespace openperf::packetio::dpdk::config
