#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_packetio_dpdk_mempool[] = "modules.packetio.dpdk.tx-mempool";

MAKE_OPTION_DATA(dpdk_secondary_memory,
                 NULL,
                 MAKE_OPT("DPDK memory pool for transmit packet buffers",
                          op_packetio_dpdk_mempool,
                          0,
                          OP_OPTION_TYPE_STRING), );

REGISTER_CLI_OPTIONS(dpdk_secondary_memory)
