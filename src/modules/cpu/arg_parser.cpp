#include "config/op_config_file.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_cpuset.hpp"
#include "cpu/arg_parser.hpp"

extern const char op_cpu_mask[];

namespace openperf::cpu::config {

std::optional<core::cpuset> core_mask()
{
    using namespace openperf::config;

    if (const auto mask =
            file::op_config_get_param<OP_OPTION_TYPE_CPUSET_STRING>(
                op_cpu_mask)) {
        return (core::cpuset_online() & core::cpuset(mask.value().c_str()));
    }

    return (std::nullopt);
}

} // namespace openperf::cpu::config
