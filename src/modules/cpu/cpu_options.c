#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_cpu_mask[] = "modules.cpu.cpu-mask";

MAKE_OPTION_DATA(
    cpu,
    NULL,
    MAKE_OPT("specifies CPU core mask for all CPU module threads, in hex",
             op_cpu_mask,
             0,
             OP_OPTION_TYPE_HEX), );

REGISTER_CLI_OPTIONS(cpu)
