#include "core/op_core.h"
#include "config/op_config_file.hpp"

const char op_packetio_dpdk_options[] = "modules.packetio.dpdk.options";
const char op_packetio_dpdk_port_ids[] = "modules.packetio.dpdk.port-ids";
const char op_packetio_dpdk_misc_worker_mask[] =
    "modules.packetio.dpdk.misc-worker-mask";
const char op_packetio_dpdk_rx_worker_mask[] =
    "modules.packetio.dpdk.rx-worker-mask";
const char op_packetio_dpdk_tx_worker_mask[] =
    "modules.packetio.dpdk.tx-worker-mask";

MAKE_OPTION_DATA(
    dpdk,
    NULL,
    MAKE_OPT("quoted, comma separated options for DPDK",
             op_packetio_dpdk_options,
             'd',
             OP_OPTION_TYPE_LIST),
    MAKE_OPT("quoted, comma separated list of port index-id mappings in the "
             "form portX=id",
             op_packetio_dpdk_port_ids,
             0,
             OP_OPTION_TYPE_MAP),
    MAKE_OPT("specifies CPU core mask for miscellaneous threads, in hex",
             op_packetio_dpdk_misc_worker_mask,
             'M',
             OP_OPTION_TYPE_HEX),
    MAKE_OPT("specifies CPU core mask for receive threads, in hex",
             op_packetio_dpdk_rx_worker_mask,
             'R',
             OP_OPTION_TYPE_HEX),
    MAKE_OPT("specifies CPU core mask for transmit threads, in hex",
             op_packetio_dpdk_tx_worker_mask,
             'T',
             OP_OPTION_TYPE_HEX), );

REGISTER_CLI_OPTIONS(dpdk)
