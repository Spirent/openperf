#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(
    dpdk, NULL,
    MAKE_OPT("quoted, comma separated options for DPDK",
             "modules.packetio.dpdk.options", 'd', OP_OPTION_TYPE_LIST),
    MAKE_OPT("enable test mode by creating loopback port pairs",
             "modules.packetio.dpdk.test-mode", 0, OP_OPTION_TYPE_NONE),
    MAKE_OPT("number of loopback port pairs for testing, defaults to 1",
             "modules.packetio.dpdk.test-portpairs", 0, OP_OPTION_TYPE_LONG),
    MAKE_OPT("quoted, comma separated list of port index-id mappings in the "
             "form portX=id",
             "modules.packetio.dpdk.port-ids", 0, OP_OPTION_TYPE_MAP),
    MAKE_OPT("disable receive queue interrupts",
             "modules.packetio.dpdk.no-rx-interrupts", 0, OP_OPTION_TYPE_NONE),
    MAKE_OPT("maximum number of receive workers per NUMA node",
             "modules.packetio.dpdk.max-rx-workers", 'R', OP_OPTION_TYPE_LONG),
    MAKE_OPT("maximum number of transmit workers per NUMA node",
             "modules.packetio.dpdk.max-tx-workers", 'T',
             OP_OPTION_TYPE_LONG), );

REGISTER_CLI_OPTIONS(dpdk)
