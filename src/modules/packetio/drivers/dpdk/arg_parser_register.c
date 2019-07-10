#include "core/icp_core.h"
#include "config/icp_config_file.h"

MAKE_OPTION_DATA(
  dpdk, NULL,
  MAKE_OPT("quoted, comma separated options for DPDK",
           "modules.packetio.dpdk.options", 'd',
           ICP_OPTION_TYPE_LIST),
  MAKE_OPT("enable test mode by creating loopback port pairs",
           "modules.packetio.dpdk.test-mode", 0,
           ICP_OPTION_TYPE_NONE),
  MAKE_OPT("number of loopback port pairs for testing, defaults to 1",
           "modules.packetio.dpdk.test-portpairs", 0, ICP_OPTION_TYPE_LONG),
  MAKE_OPT("quoted, comma separated list of port index-id mappings in the form portX=id",
           "modules.packetio.dpdk.port-ids", 0, ICP_OPTION_TYPE_MAP), );

REGISTER_CLI_OPTIONS(dpdk)
