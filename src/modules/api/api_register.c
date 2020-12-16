#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(
    api,
    NULL,
    MAKE_OPT("API service port", "modules.api.port", 'p', OP_OPTION_TYPE_LONG),
    MAKE_OPT("API service IPv4 or IPv6 address",
             "modules.api.address",
             0,
             OP_OPTION_TYPE_STRING), );

REGISTER_CLI_OPTIONS(api)
