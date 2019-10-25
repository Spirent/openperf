#include "socket/memcpy/impl.h"
#include <string.h>

namespace dpdk {
namespace impl {
void * memcpy(void *dst, const void *src, size_t n)
{
    return ::memcpy(dst,src,n);
}

}
}
