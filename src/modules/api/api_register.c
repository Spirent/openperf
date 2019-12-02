#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(api,
                 NULL,
                 MAKE_OPT("API service port",
                          "modules.api.port",
                          'p',
                          OP_OPTION_TYPE_LONG), );

REGISTER_CLI_OPTIONS(api)
