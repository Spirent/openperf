#include "core/icp_core.h"
#include "config/icp_config_file.h"

MAKE_OPTION_DATA(socket, NULL,
                 MAKE_OPT("Force removal of stale files", "modules.socket.force-unlink", 0, ICP_OPTION_TYPE_BOOL),
                 MAKE_OPT("Prefix for running multiple instances", "modules.socket.prefix", 0, ICP_OPTION_TYPE_STRING),
                 );

REGISTER_CLI_OPTIONS(socket)
