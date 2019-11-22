#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "op_common.h"

void op_exit(const char* format, ...)
{
    va_list ap;
    char* exit_msg = NULL;
    va_start(ap, format);
    (void)vasprintf(&exit_msg, format, ap);
    va_end(ap);
    fprintf(stderr, "  FATAL:%16p: %s", (void*)pthread_self(), exit_msg);
    exit(EXIT_FAILURE);
}
