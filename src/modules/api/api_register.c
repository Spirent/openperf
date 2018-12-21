#include "core/icp_core.h"

extern int api_option_handler(int opt, const char *opt_arg);

static struct icp_options_data api_options = {
    .name = "API",
    .init = NULL,
    .callback = api_option_handler,
    .options = {
        {"API service port", "api-port", 'p', true},
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(api_options)
