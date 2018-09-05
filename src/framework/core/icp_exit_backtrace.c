#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "icp_common.h"

static void print_trace(void)
{
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        fprintf(stderr, "%s\n", strs[i]);
    }
    free(strs);
}

void icp_exit(const char *format, ...)
{
    va_list ap;
    char *exit_msg = NULL;
    va_start(ap, format);
    (void)vasprintf(&exit_msg, format, ap);
    va_end(ap);
    fprintf(stderr, "  FATAL:%16p: %s", (void *)pthread_self(), exit_msg);
    print_trace();
    exit(EXIT_FAILURE);
}
