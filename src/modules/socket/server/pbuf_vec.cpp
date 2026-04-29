#include "socket/server/pbuf_vec.hpp"

namespace openperf::socket::server {

pbuf_vec::pbuf_vec() = default;

pbuf_vec::pbuf_vec(struct pbuf* pbuf, void* payload, uint16_t length)
    : m_pbuf(pbuf)
    , m_payload(payload)
{
    static_assert(sizeof(void*) == 8); /* requirement for this class */
    len(length);
}

pbuf* pbuf_vec::pbuf() const
{
    // NOLINTBEGIN(performance-no-int-to-ptr)
    return (reinterpret_cast<struct pbuf*>(reinterpret_cast<uintptr_t>(m_pbuf)
                                           & ptr_mask));
    // NOLINTEND(performance-no-int-to-ptr)
}

void* pbuf_vec::payload() const
{
    // NOLINTBEGIN(performance-no-int-to-ptr)
    return (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_payload)
                                    & ptr_mask));
    // NOLINTEND(performance-no-int-to-ptr)
}

void pbuf_vec::payload(void* addr)
{
    // NOLINTBEGIN(performance-no-int-to-ptr)
    m_payload = reinterpret_cast<void*>(
        ((static_cast<uint64_t>(len()) & 0xff) << ptr_shift)
        | (reinterpret_cast<uintptr_t>(addr) & ptr_mask));
    // NOLINTEND(performance-no-int-to-ptr)
}

uint16_t pbuf_vec::len() const
{
    // NOLINTBEGIN(performance-no-int-to-ptr)
    return (static_cast<uint16_t>(
        (reinterpret_cast<uintptr_t>(m_pbuf) >> ptr_shift) << 8
        | (reinterpret_cast<uintptr_t>(m_payload) >> ptr_shift)));
    // NOLINTEND(performance-no-int-to-ptr)
}

void pbuf_vec::len(uint16_t length)
{
    // NOLINTBEGIN(performance-no-int-to-ptr)
    m_pbuf = reinterpret_cast<struct pbuf*>(
        ((static_cast<uint64_t>(length) >> 8) << ptr_shift)
        | (reinterpret_cast<uintptr_t>(m_pbuf) & ptr_mask));
    m_payload = reinterpret_cast<void*>(
        ((static_cast<uint64_t>(length) & 0xff) << ptr_shift)
        | (reinterpret_cast<uintptr_t>(m_payload) & ptr_mask));
    // NOLINTEND(performance-no-int-to-ptr)
}

} // namespace openperf::socket::server
