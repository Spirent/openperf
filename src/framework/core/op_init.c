#include "zmq.h"

#include "core/op_common.h"
#include "core/op_init.h"
#include "core/op_log.h"
#include "core/op_modules.h"
#include "core/op_options.h"

#include "config/op_config_file.h"

void op_init(void *context, int argc, char *argv[])
{
    /*
     * First, we need to get the logging thread up and running. Look
     * for a log level option in the command line arguments, and set
     * as appropriate.
     */
    enum op_log_level log_level = op_log_level_find(argc, argv);
    op_log_level_set(log_level == OP_LOG_NONE ? OP_LOG_INFO : log_level);
    if (op_log_init(context, NULL) != 0) {
        op_exit("Logging initialization failed!");
    }

    /*
     * Do we have a configuration file?
     * Explicitly check for it (and set internal file name) here to avoid
     * ordering problems with other CLI arguments.
     */
    if (op_config_file_find(argc, argv) != 0) {
        op_exit("Failed to load configuration file!");
    }

    /*
     * Update log level if it was specified in the configuration file and
     * not on the command line.
     */
    if (log_level == OP_LOG_NONE) {
        /* Check if configuration file has a log level option. */
        char arg_string[OP_LOG_MAX_LEVEL_LENGTH];

        if (op_config_file_get_value_str("core.log.level", arg_string, OP_LOG_MAX_LEVEL_LENGTH)) {
            enum op_log_level cfg_log_level = parse_log_optarg(arg_string);
            op_log_level_set(cfg_log_level == OP_LOG_NONE ? OP_LOG_INFO : cfg_log_level);
        }
    }

    /* Parse system options */
    if (op_options_init() != 0
        || op_options_parse(argc, argv) != 0) {
        op_exit("Option parsing failed!");
    }

    /* Initialize all modules */
    if (op_modules_pre_init(context) != 0
        || op_modules_init(context) != 0
        || op_modules_post_init(context) != 0) {
        op_exit("Module initialization failed!");
    }

    /* Start anything that needs explicit starting */
    if (op_modules_start(context) != 0) {
        op_exit("Failed to start some modules!\n");
    }
}

void op_halt(void *context)
{
    zmq_ctx_shutdown(context);
    op_modules_finish();
    zmq_ctx_term(context);
}
