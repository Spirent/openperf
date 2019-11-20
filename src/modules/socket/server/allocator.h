#ifndef _OP_SOCKET_SERVER_ALLOCATOR_H_
#define _OP_SOCKET_SERVER_ALLOCATOR_H_

#include "memory/std_allocator.h"
#include "memory/allocator/free_list.h"

namespace openperf {
namespace socket {
namespace server {

typedef openperf::memory::std_allocator<uint8_t, openperf::memory::allocator::free_list> allocator;

}
}
}

#endif /* _OP_SOCKET_SERVER_ALLOCATOR_H_ */
