#ifndef _OP_PACKETIO_DPDK_ARG_PARSER_HPP_
#define _OP_PACKETIO_DPDK_ARG_PARSER_HPP_

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "packetio/drivers/dpdk/core_mask.hpp"

extern const char op_packetio_cpu_mask[];
extern const char op_packetio_dpdk_misc_worker_mask[];
extern const char op_packetio_dpdk_no_init[];
extern const char op_packetio_dpdk_options[];
extern const char op_packetio_dpdk_port_ids[];
extern const char op_packetio_dpdk_rx_worker_mask[];
extern const char op_packetio_dpdk_tx_worker_mask[];
extern const char op_packetio_dpdk_drop_tx_overruns[];

namespace openperf::packetio::dpdk::config {

std::vector<std::string> dpdk_args(); /**< Retrieve a copy of args for use */
std::vector<std::string> common_dpdk_args(); /**< Common args for all
                                                process types */

bool dpdk_disabled();

std::map<uint16_t, std::string>
dpdk_id_map(); /**< Retrieve a copy of port idx->id map */

std::optional<core_mask> packetio_mask();
std::optional<core_mask> misc_core_mask();
std::optional<core_mask> rx_core_mask();
std::optional<core_mask> tx_core_mask();

template <typename Container, typename Thing>
bool contains(const Container& c, const Thing& t)
{
    return (std::any_of(std::begin(c), std::end(c), [&](const auto& item) {
        return (item == t);
    }));
}

/*
 * Our list argument type splits arguments on commas, however some DPDK options
 * use commas to support key=value modifiers.  This function reconstructs the
 * commas so that arguments are parsed correctly.
 */
inline void add_dpdk_argument(std::vector<std::string>& args,
                              std::string_view input)
{
    if (args.empty() || input.front() == '-'
        || (!args.empty() && args.back().front() == '-')) {
        args.emplace_back(input);
    } else {
        args.back().append("." + std::string(input));
    }
}

bool dpdk_disable_lro();
bool dpdk_disable_rx_irq();
bool dpdk_drop_tx_overruns();

} /* namespace openperf::packetio::dpdk::config */

#endif /* _OP_PACKETIO_DPDK_ARG_PARSER_HPP_ */
