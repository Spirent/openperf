#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>

#include "core/icp_core.h"

#include "zmq.h"

static bool _icp_done = false;
void signal_handler(int signo __attribute__((unused)))
{
    _icp_done = true;
}

static const char * icp_pid_file_path_name = "/tmp/.com.spirent.inception/inception.pid";
static const int icp_max_proc_path_length = 12;
int find_running_inception(int pid) {
    assert(pid > 0);
    char proc_path[icp_max_proc_path_length];
    snprintf(proc_path, icp_max_proc_path_length, "/proc/%d", pid);

    DIR * proc_dir = opendir(proc_path);
    if (proc_dir) {
        /* Process exists. */
        closedir(proc_dir);
        return 0;
    }

    if (errno == ENOENT) {
        /* Directory does not exist. Not an error in this context. */
        return 1;
    }

    fprintf(stderr, "Error encountered while searching /proc for PID %d (%s).\n", pid, strerror(errno));
    return -1;
}

void handle_pid_file(void)
{
    int pid_fd = -1;
    FILE * pid_file = NULL;
    if ((pid_fd = open(icp_pid_file_path_name, O_CREAT | O_EXCL | O_RDWR)) < 0)
    {
        if (errno == EEXIST) {
            pid_file = fopen(icp_pid_file_path_name, "r");
            assert(pid_file);

            int previous_pid = 0;
            errno = 0;
            if (fscanf(pid_file, "%d", &previous_pid) != 1) {
                /* Log error and exit. Something screwy with the PID file. */
                icp_exit("Issue with inception PID file (%s): %s\n", icp_pid_file_path_name,
                         errno ? strerror(errno) : "Invalid data found");
            }

            switch (find_running_inception(previous_pid)) {
            case 0:
                /* Process exists. Log and exit. */
                icp_exit("Error: inception already running as PID %d\n", previous_pid);
                break;
            case 1:
                /* Process does not exist. Output warning about unclean shutdown and continue. */
                /* This early in the game ICP_LOG() isn't yet available. */
                printf("Warning: Previous instance of inception did not shut down cleanly.\n");
                break;
            case -1:
            default:
                /* No idea what happened. Log error and exit. */
                icp_exit("Error encountered while searching for existing instances of inception.\n");
                break;
            }

            pid_file = freopen(icp_pid_file_path_name, "w+", pid_file);
        } else {
            /* Log error and exit. Something screwy with the PID file. */
            icp_exit("Error encountered while accessing %s: (%s).\n", icp_pid_file_path_name, strerror(errno));
        }
    } else {
        pid_file = fdopen(pid_fd, "w+");
    }

    assert(pid_file);

    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);
}

extern int packetio_init(int argc, char *argv[]);

int main(int argc, char* argv[])
{
    handle_pid_file();

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

    icp_init(context, argc, argv);

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
    icp_halt(context);

    unlink(icp_pid_file_path_name);

    exit(EXIT_SUCCESS);
}
