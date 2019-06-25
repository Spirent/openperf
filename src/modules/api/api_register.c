#include "core/icp_core.h"
#include "config/icp_config_file.h"

MAKE_OPTION_DATA(api, NULL,
                 MAKE_OPT("API service port", "api.port", 'p',
                          true, ICP_OPTION_TYPE_INT),
                 );

REGISTER_MODULE_OPTIONS(api)
