#ifndef _OP_SOCKET_SERVER_ALLOCATOR_HPP_
#define _OP_SOCKET_SERVER_ALLOCATOR_HPP_

#include "memory/std_allocator.hpp"
#include "memory/allocator/free_list.hpp"

namespace openperf {
namespace socket {
namespace server {

typedef openperf::memory::std_allocator<uint8_t,
                                        openperf::memory::allocator::free_list>
    allocator;

}
} // namespace socket
} // namespace openperf

#endif /* _OP_SOCKET_SERVER_ALLOCATOR_HPP_ */
