#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(network,
                 NULL,
                 MAKE_OPT("specifies Network driver for generators and server",
                          "modules.network.drivers",
                          0,
                          OP_OPTION_TYPE_STRING), );

REGISTER_CLI_OPTIONS(network)
