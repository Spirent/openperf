#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(network,
                 NULL,
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
