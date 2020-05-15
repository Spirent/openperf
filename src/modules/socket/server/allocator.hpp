#ifndef _OP_SOCKET_SERVER_ALLOCATOR_HPP_
#define _OP_SOCKET_SERVER_ALLOCATOR_HPP_

#include "memory/std_allocator.hpp"
#include "memory/allocator/free_list.hpp"

namespace openperf::socket::server {

typedef openperf::memory::std_allocator<uint8_t,
                                        openperf::memory::allocator::free_list>
    allocator;

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_ALLOCATOR_HPP_ */
