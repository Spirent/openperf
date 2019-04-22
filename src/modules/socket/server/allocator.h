#ifndef _ICP_SOCKET_SERVER_ALLOCATOR_H_
#define _ICP_SOCKET_SERVER_ALLOCATOR_H_

#include "memory/std_allocator.h"
#include "memory/allocator/free_list.h"

namespace icp {
namespace socket {
namespace server {

typedef icp::memory::std_allocator<uint8_t, icp::memory::allocator::free_list> allocator;

}
}
}

#endif /* _ICP_SOCKET_SERVER_ALLOCATOR_H_ */
