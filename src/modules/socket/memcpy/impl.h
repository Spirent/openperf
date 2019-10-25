#ifndef _ICP_SOCKET_DPDK_MEMCPY_IMPL_H_
#define _ICP_SOCKET_DPDK_MEMCPY_IMPL_H_

#include <cstddef>

namespace dpdk {
namespace impl {

void * memcpy(void *dst, const void *src, size_t n);

}
}

#endif /* _ICP_SOCKET_DPDK_MEMCPY_IMPL_H_ */
