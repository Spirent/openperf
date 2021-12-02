#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_memory_mask[] = "modules.memory.cpu-mask";

MAKE_OPTION_DATA(
    memory,
    NULL,
    MAKE_OPT("specifies CPU core mask for all Memory module threads, in hex",
             op_memory_mask,
             0,
             OP_OPTION_TYPE_CPUSET_STRING), );

REGISTER_CLI_OPTIONS(memory)
