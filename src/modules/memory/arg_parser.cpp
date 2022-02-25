#include "config/op_config_file.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_cpuset.hpp"
#include "memory/arg_parser.hpp"

extern const char op_memory_mask[];

namespace openperf::memory::config {

std::optional<core::cpuset> core_mask()
{
    using namespace openperf::config;

    if (const auto mask =
            file::op_config_get_param<OP_OPTION_TYPE_CPUSET_STRING>(
                op_memory_mask)) {
        return (core::cpuset_online() & core::cpuset(mask.value().c_str()));
    }

    return (std::nullopt);
}

} // namespace openperf::memory::config
