#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#include "socket/unix_socket.hpp"

namespace openperf::socket {

void check_and_create_path(const std::string_view path)
{
    std::vector<std::string_view> tokens;
    /* tear down the path into tokens */
    std::string_view delimiters("/");
    size_t beg = 0, pos = 0;
    while ((beg = path.find_first_not_of(delimiters, pos))
           != std::string::npos) {
        pos = path.find_first_of(delimiters, beg + 1);
        tokens.emplace_back(path.substr(beg, pos - beg));
    }

    if (tokens.size() == 1) return; /* nothing to do */

    /* and build it back up, creating necessary directories as we go */
    std::string current(path[0] == '/' ? "/" : "");
    for (size_t idx = 0; idx < tokens.size() - 1; idx++) {
        if (idx != 0) current.append("/");
        current.append(tokens[idx]);
        if (mkdir(current.c_str(), S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO) == -1
            && errno != EEXIST) {
            throw std::runtime_error("Could not create path for unix socket: "
                                     + std::string(strerror(errno)));
        }
    }
}

unix_socket::unix_socket(const std::string_view path, int type)
    : m_path(path)
    , m_fd(::socket(AF_UNIX, type, 0))
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not create unix socket: "
                                 + std::string(strerror(errno)));
    }

    check_and_create_path(path);

    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    std::strncpy(addr.sun_path, m_path.c_str(), sizeof(addr.sun_path));

    if (bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        if (errno == EADDRINUSE) {
            throw std::runtime_error(
                "Could not bind to unix socket at \"" + m_path
                + "\". Either OpenPerf is already "
                + "running or did not shut down cleanly. "
                + "Use --modules.socket.force-unlink option "
                + "to force launch.");
        } else {
            throw std::runtime_error("Could not bind to unix socket "
                                     + std::to_string(m_fd) + ": "
                                     + std::string(strerror(errno)));
        }
    }
}

unix_socket::~unix_socket()
{
    close(m_fd);
    unlink(m_path.c_str());
}

int unix_socket::get() { return (m_fd); }

} // namespace openperf::socket
