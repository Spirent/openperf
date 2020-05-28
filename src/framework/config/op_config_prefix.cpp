#include "config/op_config_file.hpp"
#include "config/op_config_prefix.hpp"

namespace openperf::config {

std::optional<std::string> get_prefix()
{
    /*
     * XXX: no way around duplicating the string here. We have to handle the
     * registration in C due to VLA usage in the C11 option implementation.
     */
    return (file::op_config_get_param<OP_OPTION_TYPE_STRING>("core.prefix"));
}

} // namespace openperf::config
