#include "core/icp_common.h"
#include "core/icp_init.h"
#include "core/icp_log.h"
#include "core/icp_modules.h"
#include "core/icp_options.h"

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

    /* Parse system options */
    if (icp_options_init() != 0
        || icp_options_parse(argc, argv) != 0) {
        icp_exit("Option parsing failed!");
    }

    /* Initialize all modules */
    if (icp_modules_init(context) != 0) {
        icp_exit("Module initialization failed!");
    }
}
