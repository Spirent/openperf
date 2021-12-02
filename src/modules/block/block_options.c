#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_block_mask[] = "modules.block.cpu-mask";

MAKE_OPTION_DATA(
    block,
    NULL,
    MAKE_OPT("specifies CPU core mask for all Block module threads, in hex",
             op_block_mask,
             0,
             OP_OPTION_TYPE_HEX), );

REGISTER_CLI_OPTIONS(block)
