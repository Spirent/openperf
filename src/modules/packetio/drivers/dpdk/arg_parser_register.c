#include "core/icp_core.h"

extern int dpdk_arg_parse_init();
extern int dpdk_arg_parse_handler(int opt, const char *opt_arg);

struct icp_options_data dpdk_options = {
    .name = "DPDK",
    .init = dpdk_arg_parse_init,
    .callback = dpdk_arg_parse_handler,
    .options = {
        {"quoted, comma separated options for DPDK", "dpdk", 'd', true},
        {"configure ethernet ring devices", "dpdk-test-mode", 0, false},
        {"specify number of connected port pairs", "dpdk-test-portpairs", 0, true},
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(dpdk_options)
