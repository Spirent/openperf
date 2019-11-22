#include "core/op_core.h"
#include "config/op_config_file.hpp"

MAKE_OPTION_DATA(socket, NULL,
                 MAKE_OPT("Force removal of stale files",
                          "modules.socket.force-unlink", 'f',
                          OP_OPTION_TYPE_NONE),
                 MAKE_OPT("Prefix for running multiple instances",
                          "modules.socket.prefix", 0, OP_OPTION_TYPE_STRING), );

REGISTER_CLI_OPTIONS(socket)
