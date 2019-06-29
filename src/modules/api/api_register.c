#include "core/icp_core.h"
#include "config/icp_config_file.h"

MAKE_OPTION_DATA(api, NULL,
                 MAKE_OPT("API service port", "modules.api.port", 'p',
                          1, ICP_OPTION_TYPE_LONG),
                 );

REGISTER_CLI_OPTIONS(api)
