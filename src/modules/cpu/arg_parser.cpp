#include <future>

#include "config/op_config_file.hpp"
#include "core/op_core.h"
#include "core/op_log.h"
#include "cpu/arg_parser.hpp"

extern const char op_cpu_mask[];

namespace openperf::cpu::config {

core_mask all_cores()
{
    /*
     * We have to check online cpus in a separate thread because
     * the DPDK init process will lock the main thread to a core.
     */
    auto cores = std::async(op_get_cpu_online).get();
    OP_LOG(OP_LOG_DEBUG, "Detected %zu cores", cores.size());

    /* Turn vector of ints into core mask */
    auto mask = core_mask{};
    std::for_each(std::begin(cores), std::end(cores), [&](auto value) {
        mask.set(value);
    });

    return (mask);
}

core_mask available_cores()
{
    auto cores = all_cores();
    if (const auto mask =
            openperf::config::file::op_config_get_param<OP_OPTION_TYPE_HEX>(
                op_cpu_mask)) {
        return (cores & core_mask{*mask});
    }

    return (cores);
}

} // namespace openperf::cpu::config
