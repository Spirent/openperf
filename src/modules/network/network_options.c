#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_network_mask[] = "modules.network.cpu-mask";
const char op_network_driver[] = "modules.network.driver";
const char op_network_op_timeout[] = "modules.network.operation-timeout";

MAKE_OPTION_DATA(
    network,
    NULL,
    MAKE_OPT("specifies CPU core mask for all Network module threads, in hex",
             "modules.network.cpu-mask",
             0,
             OP_OPTION_TYPE_HEX),
    MAKE_OPT("specifies Network driver for generators and server",
             "modules.network.driver",
             0,
             OP_OPTION_TYPE_STRING),
    MAKE_OPT("specifies Network operation timeout in "
             "microseconds, default 1000000 (1s)",
             "modules.network.operation-timeout",
             0,
             OP_OPTION_TYPE_LONG), );

REGISTER_CLI_OPTIONS(network)
