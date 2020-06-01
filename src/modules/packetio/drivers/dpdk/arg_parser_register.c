#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(
    dpdk,
    NULL,
    MAKE_OPT("quoted, comma separated options for DPDK",
             "modules.packetio.dpdk.options",
             'd',
             OP_OPTION_TYPE_LIST),
    MAKE_OPT("enable test mode by creating loopback port pairs",
             "modules.packetio.dpdk.test-mode",
             0,
             OP_OPTION_TYPE_NONE),
    MAKE_OPT("number of loopback port pairs for testing, defaults to 1",
             "modules.packetio.dpdk.test-portpairs",
             0,
             OP_OPTION_TYPE_LONG),
    MAKE_OPT("quoted, comma separated list of port index-id mappings in the "
             "form portX=id",
             "modules.packetio.dpdk.port-ids",
             0,
             OP_OPTION_TYPE_MAP),
    MAKE_OPT("disable receive queue interrupts",
             "modules.packetio.dpdk.no-rx-interrupts",
             0,
             OP_OPTION_TYPE_NONE),
    MAKE_OPT("specifies CPU core mask for miscellaneous threads, in hex",
             "modules.packetio.dpdk.misc-worker-mask",
             'M',
             OP_OPTION_TYPE_HEX),
    MAKE_OPT("specifies CPU core mask for receive threads, in hex",
             "modules.packetio.dpdk.rx-worker-mask",
             'R',
             OP_OPTION_TYPE_HEX),
    MAKE_OPT("specifies CPU core mask for transmit threads, in hex",
             "modules.packetio.dpdk.tx-worker-mask",
             'T',
             OP_OPTION_TYPE_HEX), );

REGISTER_CLI_OPTIONS(dpdk)
