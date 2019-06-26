#include "core/icp_core.h"
#include "config/icp_config_file.h"

MAKE_OPTION_DATA(socket, NULL,
                 MAKE_OPT("Force removal of stale files", "socket.force-unlink", 0, 0, ICP_OPTION_TYPE_NONE),
                 MAKE_OPT("Prefix for running multiple instances", "socket.prefix", 0, 1, ICP_OPTION_TYPE_STRING),
                 );

REGISTER_MODULE_OPTIONS(socket)
