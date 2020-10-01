#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(
    memory,
    NULL,
    MAKE_OPT("specifies CPU core mask for all Memory module threads, in hex",
             "modules.memory.cpu-mask",
             0,
             OP_OPTION_TYPE_HEX), );

REGISTER_CLI_OPTIONS(memory)
