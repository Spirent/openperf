#include "core/icp_core.h"

extern int api_server_option_handler(int opt, const char *opt_arg);

static struct icp_options_data api_server_options = {
    .name = "API-SERVER",
    .init = NULL,
    .callback = api_server_option_handler,
    .options = {
        {"Force removal of stale files", "force-unlink", 0, 0},
        {"Prefix for running multiple instances", "prefix", 0, 1},
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(api_server_options)
