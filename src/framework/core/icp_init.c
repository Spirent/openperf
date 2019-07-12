#include "zmq.h"

#include "core/icp_common.h"
#include "core/icp_init.h"
#include "core/icp_log.h"
#include "core/icp_modules.h"
#include "core/icp_options.h"

#include "config/icp_config_file.h"

void icp_init(void *context, int argc, char *argv[])
{
    /*
     * First, we need to get the logging thread up and running. Look
     * for a log level option in the command line arguments, and set
     * as appropriate.
     */
    enum icp_log_level log_level = icp_log_level_find(argc, argv);
    icp_log_level_set(log_level == ICP_LOG_NONE ? ICP_LOG_INFO : log_level);
    if (icp_log_init(context, NULL) != 0) {
        icp_exit("Logging initialization failed!");
    }

    /*
     * Do we have a configuration file?
     * Explicitly check for it (and set internal file name) here to avoid
     * ordering problems with other CLI arguments.
     */
    if (icp_config_file_find(argc, argv) != 0) {
        icp_exit("Failed to load configuration file!");
    }

    /*
     * Update log level if it was specified in the configuration file and
     * not on the command line.
     */
    if (log_level == ICP_LOG_NONE) {
        /* Check if configuration file has a log level option. */
        char arg_string[ICP_LOG_MAX_LEVEL_LENGTH];

        if (icp_config_file_get_value_str("core.log.level", arg_string, ICP_LOG_MAX_LEVEL_LENGTH)) {
            enum icp_log_level cfg_log_level = parse_log_optarg(arg_string);
            icp_log_level_set(cfg_log_level == ICP_LOG_NONE ? ICP_LOG_INFO : cfg_log_level);
        }
    }

    /* Parse system options */
    if (icp_options_init() != 0
        || icp_options_parse(argc, argv) != 0) {
        icp_exit("Option parsing failed!");
    }

    /* Initialize all modules */
    if (icp_modules_pre_init(context) != 0
        || icp_modules_init(context) != 0
        || icp_modules_post_init(context) != 0) {
        icp_exit("Module initialization failed!");
    }

    /* Start anything that needs explicit starting */
    if (icp_modules_start(context) != 0) {
        icp_exit("Failed to start some modules!\n");
    }
}

void icp_halt(void *context)
{
    zmq_ctx_shutdown(context);
    icp_modules_finish();
    zmq_ctx_term(context);
}
