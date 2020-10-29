#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <net/if.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <poll.h>
#include <assert.h>
#include <string>

#include "framework/core/op_log.h"
#include "internal_utils.hpp"

namespace openperf::network::internal {

char* get_mac_address(const char* ifname)
{
    int s;
    ifreq buffer;
    unsigned char* ptr = NULL;
    char* mac_addr = NULL;

    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Could not open inet dgram socket: %s\n",
               strerror(errno));
        return NULL;
    }

    memset(&buffer, 0, sizeof(buffer));
    strcpy(buffer.ifr_name, ifname);

    if (ioctl(s, SIOCGIFHWADDR, &buffer) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Could not get SIOCGIFHWADDR for %s: %s\n",
               ifname,
               strerror(errno));
        close(s);
        return NULL;
    }

    close(s);

    ptr = (unsigned char*)&buffer.ifr_hwaddr.sa_data;

    asprintf(&mac_addr,
             "%02x:%02x:%02x:%02x:%02x:%02x",
             *ptr,
             *(ptr + 1),
             *(ptr + 2),
             *(ptr + 3),
             *(ptr + 4),
             *(ptr + 5));

    return mac_addr;
}

bool ipv6_link_local_requires_scope() { return true; }

/**
 * Find neighbor interface from IPv6 neighbor cache.
 *
 * @param addr
 *  The neighbor IPv6 address.
 *
 * @return
 *  The interace index if successful or -1 if fail.
 */
int ipv6_get_neighbor_ifindex_from_cache(const in6_addr* addr)
{
    int sock;
    int result = -1;
    sockaddr_nl la;
    struct
    {
        nlmsghdr n;
        ndmsg r;
    } req;
    uint8_t* resp_buf = NULL;
    size_t resp_buf_size = 8192;
    msghdr msghdr;
    iovec iov;

    memset(&la, 0, sizeof(la));
    la.nl_family = AF_NETLINK;
    la.nl_pid = getpid();

    if ((sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to open netlink socket.  %s\n",
               strerror(errno));
        return -1;
    }

    if (bind(sock, (sockaddr*)&la, sizeof(la)) < 0) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to bind netlink socket.  %s\n",
               strerror(errno));
        goto done;
    }

    if ((resp_buf = (uint8_t*)calloc(1, resp_buf_size)) == NULL) {
        OP_LOG(OP_LOG_ERROR, "Failed allocating resp bufer.\n");
        goto done;
    }

    // fill in the netlink message header
    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(ndmsg));
    req.n.nlmsg_type = RTM_GETNEIGH;
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

    // fill in the netlink message GETNEIGH
    req.r.ndm_family = AF_INET6;
    req.r.ndm_state = NUD_REACHABLE;
    la.nl_family = AF_NETLINK;
    la.nl_pid = 0;

    memset(&iov, 0, sizeof(iov));
    iov.iov_base = &req;
    iov.iov_len = req.n.nlmsg_len;

    memset(&msghdr, 0, sizeof(msghdr));
    msghdr.msg_iov = &iov;
    msghdr.msg_iovlen = 1;
    msghdr.msg_name = &la;
    msghdr.msg_namelen = sizeof(la);

    if (sendmsg(sock, &msghdr, 0) < 0) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to send netlink request.  %s\n",
               strerror(errno));
        goto done;
    }

    while (1) {
        nlmsghdr* msg_ptr;
        int len;
        pollfd fds[1];

        fds[0].fd = sock;
        fds[0].events = POLLIN;
        fds[0].revents = 0;

        if (poll(fds, 1, 5000) != 1) {
            OP_LOG(OP_LOG_ERROR, "Timeout waiting for netlink response.\n");
            break;
        }

        iov.iov_base = resp_buf;
        iov.iov_len = resp_buf_size;

        len = recvmsg(sock, &msghdr, MSG_DONTWAIT);
        if (len < 0) break;
        if ((size_t)len < sizeof(*msg_ptr)) continue;

        for (msg_ptr = (nlmsghdr*)resp_buf; NLMSG_OK(msg_ptr, (size_t)len);
             msg_ptr = NLMSG_NEXT(msg_ptr, len)) {
            if (msg_ptr->nlmsg_type == NLMSG_DONE) { goto done; }
            if (msg_ptr->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr* nlmsgerr =
                    (struct nlmsgerr*)NLMSG_DATA(msg_ptr);
                if (msg_ptr->nlmsg_len < NLMSG_LENGTH(sizeof(nlmsgerr))) {
                    OP_LOG(OP_LOG_ERROR, "NLMSG_ERROR too short\n");
                } else {
                    OP_LOG(OP_LOG_ERROR, "NLMSG_ERROR %d\n", nlmsgerr->error);
                }
                goto done;
            }
            size_t ndm_len = RTM_PAYLOAD(msg_ptr);
            ndmsg* ndm = (ndmsg*)NLMSG_DATA(msg_ptr);
            rtattr* rta;
            int found_addr = 0;
            for (rta = RTM_RTA(ndm); RTA_OK(rta, ndm_len);
                 rta = RTA_NEXT(rta, ndm_len)) {
                if (rta->rta_type == NDA_DST) {
                    in6_addr* dst_addr = (in6_addr*)RTA_DATA(rta);
                    if (memcmp(addr, dst_addr, sizeof(*addr)) == 0) {
                        found_addr = 1;
                    }
                } else if (rta->rta_type == NDA_LLADDR) {
                    if (found_addr) {
                        /* MAC address */
                        result = ndm->ndm_ifindex;
                        break;
                    }
                }
            }
        }
    }

done:
    if (resp_buf) { free(resp_buf); }
    close(sock);
    return result;
}

/**
 * Execute new process.
 *
 * @param cmd
 *   The excutable to run.   If full path is not specified the PATH will be
 * searched.
 * @param argv
 *   The executable arguments.  The 1st argument should be the executable name.
 * @param slient
 *   If true, the stderr/stdout will be redirected to /dev/null
 *
 * @return
 *   The pid of the new process.
 */
pid_t exec_cmd(const char* cmd, char* const argv[], bool silent)
{
    pid_t pid = fork();

    if (pid == 0) {
        /* Child process */
        if (silent) {
            int fd = open("/dev/null", O_WRONLY);
            if (fd) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
        execvp(cmd, argv);
        exit(-1);
    } else if (pid > 0) {
        /* Parent process */
        return pid;
    } else {
        /* Failed */
        return -1;
    }
}

struct pinger
{
    pid_t pid;
    char ifname[IF_NAMESIZE];
    int exited;
    int exit_code;
};

/**
 * Start a ping command.
 *
 * @param ifname
 *  The interface name to ping ont.
 * @param addr
 *  The IPv6 address to ping.
 *
 * @return
 *  A struct pinger for the ping session if successful or NULL if an error
 * occurs.
 */
pinger* start_pinger(const char* ifname, const in6_addr* addr)
{
    struct pinger* pinger;
    std::string ping_cmd = "ping6";
    char addr_str[INET6_ADDRSTRLEN];

    if (inet_ntop(AF_INET6, addr, addr_str, INET6_ADDRSTRLEN) == NULL) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to convert IPv6 address to string.  %s\n",
               strerror(errno));
        return NULL;
    }

    char* const ping_args[] = {(char*)ping_cmd.c_str(),
                               (char*)"-I",
                               (char*)ifname,
                               (char*)"-c",
                               (char*)"1",
                               addr_str,
                               NULL};

    pinger = (struct pinger*)calloc(1, sizeof(*pinger));
    if (!pinger) {
        OP_LOG(OP_LOG_ERROR, "Failed to allocate memory for pinger\n");
        return NULL;
    }

    strncpy(pinger->ifname, ifname, IF_NAMESIZE);
    pinger->ifname[IF_NAMESIZE - 1] = 0;

    if ((pinger->pid = exec_cmd(ping_cmd.c_str(), ping_args, true)) < 0) {
        OP_LOG(OP_LOG_ERROR, "Failed running %s command.\n", ping_cmd.c_str());
        free(pinger);
        return NULL;
    }

    return pinger;
}

/**
 * Wait for pingers to exit.
 *
 * @param pingers
 *  Array of pingers to wait for.
 * @param n_pingers
 *  The number of pingers in the array.
 * @param n_wait
 *  The number of pingers to wait to exit.
 *
 * @return
 *  The index of the pinger which was successful, or -1 if no pingers were
 * successful.
 */
int wait_pingers(pinger* pingers[], size_t n_pingers, size_t n_wait)
{
    size_t i;
    int result = -1;
    int retry = 2;

    /* Reap any processes that have already died */
    while (retry && n_wait) {
        for (i = 0; i < n_pingers && n_wait > 0; ++i) {
            pinger* pinger = pingers[i];
            if (pinger->exited) continue;
            int status = 0;
            pid_t wait_result = waitpid(pinger->pid, &status, WNOHANG);
            if (wait_result == 0) {
                /* Still running */
            } else if (wait_result == (pid_t)-1) {
                if (errno == ECHILD || errno == EINVAL) {
                    pinger->exited = 1;
                    pinger->exit_code = -1;
                    --n_wait;
                }
            } else {
                if (WIFEXITED(status)) {
                    pinger->exited = 1;
                    pinger->exit_code = WEXITSTATUS(status);
                    --n_wait;
                    if (pinger->exit_code == 0 && result < 0) result = i;
                }
            }
        }
        --retry;
        usleep(10000);
    }

    /* If still didn't find enough processes, then just wait in order for
     * processes to exit. */
    for (i = 0; i < n_pingers && n_wait > 0; ++i) {
        pinger* pinger = pingers[i];
        if (pinger->exited) continue;
        int status = 0;
        pid_t wait_result = waitpid(pinger->pid, &status, 0);
        if (wait_result == 0) {
            /* Still running */
        } else if (wait_result == (pid_t)-1) {
            if (errno == ECHILD || errno == EINVAL) {
                pinger->exited = 1;
                pinger->exit_code = -1;
                --n_wait;
            }
        } else {
            if (WIFEXITED(status)) {
                pinger->exited = 1;
                pinger->exit_code = WEXITSTATUS(status);
                --n_wait;
                if (pinger->exit_code == 0 && result < 0) result = i;
            }
        }
    }

    return result;
}

/**
 * Delete the pinger.
 *
 * @param pinger
 *  The pinger object to delete.
 */
void delete_pinger(pinger* pinger)
{
    if (!pinger->exited) {
        OP_LOG(
            OP_LOG_ERROR,
            "Pinger for interface %s was still running when it was deleted\n",
            pinger->ifname);
    }
    free(pinger);
}

/**
 * Delete all pingers which have completed and remove them from the array.
 *
 * @param pingers
 *  Array of pingers to wait for.
 * @param n_pingers
 *  The number of pingers in the array.
 *
 * @return
 *  The number of pingers left after completed pingers have been deleted.
 */
int delete_completed_pingers(pinger* pingers[], size_t n_pingers)
{
    size_t i, j;

    for (i = 0, j = 0; i < n_pingers; ++i) {
        if (!pingers[i]->exited) {
            if (i != j) {
                assert(pingers[j] == NULL);
                pingers[j] = pingers[i];
                pingers[i] = NULL;
            }
            ++j;
        } else {
            delete_pinger(pingers[i]);
            pingers[i] = NULL;
        }
    }
    return j;
}

/**
 * Find neighbor interface using ping.
 *
 * PROTO_ICMP/PROTO_ICMPV6 require root user permissions on most linux
 * systems and writing IPv6 ping like behavior was more difficult
 * than expected, so just call ping6 executable which is setuid.
 *
 * @param addr
 *  The neighbor IPv6 address.
 *
 * @return
 *  The interace index if successful or -1 if fail.
 */
int ipv6_get_neighbor_ifindex_from_ping_exec(const in6_addr* addr)
{
    struct if_nameindex *if_nidxs, *intf;
    pinger* pingers[4];
    size_t n_pingers = 0, i = 0;
    size_t max_pingers = sizeof(pingers) / sizeof(pingers[0]);
    int ifindex = -1;
    int wait_result;

    if_nidxs = if_nameindex();
    if (if_nidxs == NULL) {
        OP_LOG(OP_LOG_ERROR,
               "Failed getting interface names.  %s\n",
               strerror(errno));
        return -1;
    }

    /* Send ping out all interfaces until it is successful */
    for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL;
         intf++) {
        if (strcmp(intf->if_name, "lo") == 0) continue;
        if (n_pingers >= max_pingers) {
            /* Reached maximum number of concurrent pingers, so need to wait for
             * one to exit.  */
            if ((wait_result = wait_pingers(pingers, n_pingers, 1)) >= 0) {
                ifindex = if_nametoindex(pingers[wait_result]->ifname);
                break;
            }
            n_pingers = delete_completed_pingers(pingers, n_pingers);
        }
        pinger* pinger = start_pinger(intf->if_name, addr);
        if (!pinger) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed to start ping on interface %s\n",
                   intf->if_name);
            continue;
        }
        pingers[n_pingers++] = pinger;
    }

    wait_result = wait_pingers(pingers, n_pingers, n_pingers);
    if (ifindex < 0) ifindex = if_nametoindex(pingers[wait_result]->ifname);

    for (i = 0; i < n_pingers; ++i) { delete_pinger(pingers[i]); }

    if_freenameindex(if_nidxs);

    return ifindex;
}

int ipv6_get_neighbor_ifindex_from_ping(const in6_addr* addr)
{
    int ifindex = -1;

    /* Try using ICMPv6 socket */
    if ((ifindex = ipv6_get_neighbor_ifindex_from_ping_socket(addr)) >= 0) {
        return ifindex;
    }

    /* Use ping6 if don't have authority to use ICMPv6 socket */
    if (errno != EPERM) { return ifindex; }

    if ((ifindex = ipv6_get_neighbor_ifindex_from_ping_exec(addr)) >= 0) {
        return ifindex;
    }

    return ifindex;
}

/*
 * Get the interface index of the IPv6 neighbor address.
 */
int ipv6_get_neighbor_ifindex(const in6_addr* addr)
{
    int ifindex;

    if ((ifindex = ipv6_get_neighbor_ifindex_from_cache(addr)) >= 0) {
        return ifindex;
    }

    /* If IPv6 link local address is not in neighbor cache, then use ping
     * to find the interface the neighbor is connected to.
     */
    if ((ifindex = ipv6_get_neighbor_ifindex_from_ping(addr)) >= 0) {
        return ifindex;
    }

    return -1;
}

} // namespace openperf::network::internal
