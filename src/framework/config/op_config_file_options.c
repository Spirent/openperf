#include "core/op_core.h"

/*
 * No need to register a callback here.
 * The framework explicitly checks for a configuration file argument
 * and sets the filename variable accordingly.
 */
static struct op_options_data config_file_options = {
    .name = "CONFIG-FILE",
    .init = NULL,
    .callback = NULL,
    .options = {
        {"File to use to configure OpenPerf", "config", 'c', OP_OPTION_TYPE_STRING},
        { 0, 0, 0, 0 },
    },
};

REGISTER_OPTIONS(config_file_options)
