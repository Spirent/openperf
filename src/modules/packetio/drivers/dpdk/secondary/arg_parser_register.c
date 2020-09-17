#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_packetio_dpdk_tx_queues[] = "modules.packetio.dpdk.tx-queues";

MAKE_OPTION_DATA(
    dpdk_secondary,
    NULL,
    MAKE_OPT("comma separated list or dashed range of port transmit queues "
             "in the form portX=x,y-z",
             op_packetio_dpdk_tx_queues,
             0,
             OP_OPTION_TYPE_MAP), );

REGISTER_CLI_OPTIONS(dpdk_secondary)
