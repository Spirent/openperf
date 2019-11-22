#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "core/op_core.h"

#include "zmq.h"

static bool _op_done = false;
void signal_handler(int signo __attribute__((unused))) { _op_done = true; }

int main(int argc, char* argv[])
{
    op_thread_setname("op_main");

    /* Block child threads from intercepting SIGINT or SIGTERM */
    sigset_t newset, oldset;
    sigemptyset(&newset);
    sigaddset(&newset, SIGINT);
    sigaddset(&newset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    void* context = zmq_ctx_new();
    if (!context) { op_exit("Could not initialize ZeroMQ context!"); }

    op_init(context, argc, argv);

    /* Install our signal handler so we can property shut ourselves down */
    struct sigaction s;
    s.sa_handler = signal_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
    sigaction(SIGTERM, &s, NULL);

    /* Restore the default signal mask */
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    /* Block until we're done ... */
    sigset_t emptyset;
    sigemptyset(&emptyset);
    while (!_op_done) { sigsuspend(&emptyset); }

    /* ... then clean up and exit. */
    op_halt(context);

    exit(EXIT_SUCCESS);
}
