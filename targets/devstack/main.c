#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "core/icp_core.h"

#include "zmq.h"

static bool _icp_done = false;
void signal_handler(int signo __attribute__((unused)))
{
    _icp_done = true;
}

extern int packetio_init(int argc, char *argv[]);

int main(int argc, char* argv[])
{
    icp_thread_setname("icp_main");

    /* Block child threads from intercepting SIGINT or SIGTERM */
    sigset_t newset, oldset;
    sigemptyset(&newset);
    sigaddset(&newset, SIGINT);
    sigaddset(&newset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    void *context = zmq_ctx_new();
    if (!context) {
        icp_exit("Could not initialize ZeroMQ context!");
    }

    icp_log_level_set(ICP_LOG_INFO);
    int ret = icp_log_init(context, NULL);
    if (ret != 0) {
        icp_exit("Logging initialization failed!");
    }

    /* Aborts on error */
    packetio_init(argc, argv);

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
    while (!_icp_done) {
        sigsuspend(&emptyset);
    }

    /* ... then clean up and exit. */
    zmq_ctx_term(context);

    exit(EXIT_SUCCESS);
}
