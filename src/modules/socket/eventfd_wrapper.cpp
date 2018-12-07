#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/eventfd.h>
#include <unistd.h>

#include "socket/eventfd_wrapper.h"

namespace icp {
namespace sock {

eventfd_wrapper::eventfd_wrapper(unsigned initval, int flags)
    : m_fd(eventfd(initval, flags))
    , m_flags(flags)
    , m_value(initval)
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }
}

eventfd_wrapper::~eventfd_wrapper()
{
    close(m_fd);
}

int eventfd_wrapper::get()
{
    return (m_fd);
}

uint64_t eventfd_wrapper::read()
{
    uint64_t counter = 0;
    if (eventfd_read(m_fd, &counter) == -1) {
        throw std::runtime_error("Could not read eventfd: "
                                 + std::string(strerror(errno)));
    }
    m_value = ((m_flags & EFD_SEMAPHORE) ? m_value - 1 : 0);
    return (counter);
}

void eventfd_wrapper::write(uint64_t value)
{
    if (eventfd_write(m_fd, value) == -1) {
        throw std::runtime_error("Could not write eventfd: "
                                 + std::string(strerror(errno)));
    }
    m_value += value;
}

void eventfd_wrapper::update(bool readable, bool writable)
{
    /* convert our bools to a switchable int value */
    switch (readable * 2 + writable) {
    case 1:
        /* writable only */
        while (m_value != 0) read();
        break;
    case 2:
        /* readable only */
        write(eventfd_max - m_value);
        break;
    case 3:
        /* readable and writable */
        switch (m_value) {
        case 0:
            write(1);
            break;
        case eventfd_max:
            read();
            break;
        default:
            break;
        }
    default:
        /* not readable and not writable is a case eventfd can't handle */
        break;
    }
}

}
}
