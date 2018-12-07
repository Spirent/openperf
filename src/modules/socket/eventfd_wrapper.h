#ifndef _ICP_SOCKET_EVENTFD_WRAPPER_H_
#define _ICP_SOCKET_EVENTFD_WRAPPER_H_

#include <cstdint>
#include <limits>

namespace icp {
namespace sock {

class eventfd_wrapper
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    int m_fd;
    int m_flags;
    uint64_t m_value;

public:
    eventfd_wrapper(unsigned initval = 0, int flags = 0);
    ~eventfd_wrapper();

    int get();

    uint64_t read();
    void write(uint64_t);

    void update(bool readable, bool writable);
};

}
}

#endif /* _ICP_SOCKET_EVENTFD_WRAPPER_H_ */
