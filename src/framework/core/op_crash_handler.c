#include <stdlib.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include "core/op_common.h"

struct sig_name_pair {
    int signo;
    const char *name;
};

struct sig_name_pair crash_sig_table[] = {
    {SIGABRT, "SIGABRT"},
    {SIGBUS, "SIGBUS"},
    {SIGFPE, "SIGFPE"},
    {SIGILL, "SIGILL"},
    {SIGSEGV, "SIGSEGV"},
};

const int crash_sig_table_size = sizeof(crash_sig_table)/sizeof(crash_sig_table[0]);

static const char *signal_id_to_name(int signo) {
    int i;
    for (i=0; i<crash_sig_table_size; ++i) {
        if (crash_sig_table[i].signo == signo)
            return crash_sig_table[i].name;
    }
    return "?";
}

static void crash_handler(int signo, siginfo_t *siginfo, void *context) {
    const int max_symbols = 100;
    void *symbols[max_symbols];
    int symbol_count = backtrace(symbols, max_symbols);

    fprintf(stderr, "Crash handler received signal %s(%d)\n", signal_id_to_name(signo), signo);
    if (siginfo) {
        if (signo == SIGSEGV || signo == SIGBUS) {
            fprintf(stderr, "Fault address %p\n", siginfo->si_addr);
        }
        else if (signo == SIGILL || signo == SIGFPE) {
            fprintf(stderr, "Fault instruction address %p\n", siginfo->si_addr);
        }
    }
    fflush(stderr);
    backtrace_symbols_fd(symbols, symbol_count, STDERR_FILENO);
}

void op_init_crash_handler() {
    /* Install crash handler to print stack trace */
    struct sigaction s;
    sigemptyset(&s.sa_mask);
    s.sa_sigaction = crash_handler;
    s.sa_flags = SA_RESTART | SA_SIGINFO | SA_RESETHAND;
    int i;
    for (i=0; i<crash_sig_table_size; ++i) {
        sigaction(crash_sig_table[i].signo, &s, NULL);
    }
}
