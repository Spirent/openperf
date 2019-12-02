#include "config/op_config_file.h"
#include "core/op_core.h"

MAKE_OPTION_DATA(main,
                 NULL,
                 MAKE_OPT("Specify port id for analyzer; defaults to \"port0\"",
                          "main.sink-port",
                          'P',
                          OP_OPTION_TYPE_STRING), );

REGISTER_CLI_OPTIONS(main)
