#include "core/icp_core.h"

extern int socket_option_handler(int opt, const char *opt_arg);

struct icp_options_data socket_options = {
    .name = "SOCKET",
    .init = NULL,
    .callback = socket_option_handler,
    .options = {
        { "prefix for running multiple ICP instances", "prefix", 'm', true },
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(socket_options)
