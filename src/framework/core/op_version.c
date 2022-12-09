#include "core/op_core.h"
#include "sys/utsname.h"
#include <ctype.h>

#ifndef BUILD_VERSION
#error BUILD_VERSION must be defined
#endif

void make_lower_case(char* input)
{
    for (; *input; input++) *input = tolower(*input);
}

static int version_option_handler(int opt __attribute__((unused)),
                                  const char* opt_arg __attribute__((unused)))
{
    struct utsname info;
    if (uname(&info) < 0) exit(EXIT_FAILURE);
    make_lower_case(info.sysname);
    make_lower_case(info.machine);

    FILE* output = stderr;

    fprintf(output,
            "Spirent OpenPerf, version %s (%s-%s)\n",
            BUILD_VERSION,
            info.machine,
            info.sysname);
    fprintf(output, "Copyright (C) 2022 Spirent Communications\n");
    exit(EXIT_SUCCESS);
}

static struct op_options_data version_options = {
    .name = "VERSION",
    .init = NULL,
    .callback = version_option_handler,
    .options =
        {
            {"Version", "version", 'v', OP_OPTION_TYPE_NONE},
            {0, 0, 0, 0},
        },
};

REGISTER_OPTIONS(version_options)
