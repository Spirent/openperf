#include "core/icp_core.h"

extern int config_file_option_handler(int opt, const char *opt_arg);

static struct icp_options_data config_file_options = {
    .name = "CONFIG-FILE",
    .init = NULL,
    .callback = config_file_option_handler,
    .options = {
        {"File to use to configure Inception", "config", 'c', ICP_OPTION_TYPE_STRING},
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(config_file_options)
