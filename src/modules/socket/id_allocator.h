#ifndef _ICP_SOCKET_ID_ALLOCATOR_H_
#define _ICP_SOCKET_ID_ALLOCATOR_H_

#include <bitset>
#include <cassert>
#include <optional>

namespace icp {
namespace sock {

template <typename T, std::size_t N>
class id_allocator
{
    std::bitset<N> m_ids;
    T m_offset;

public:
    id_allocator(T offset = 0)
        : m_offset(offset)
    {
        m_ids.set();  /* set all bits to true */
    }

    std::optional<T> acquire()
    {
        /*
         * XXX: _Find_first is a non-standard extension, but
         * it does exactly what we want and uses fancy CPU
         * instructions to do it, so it's fast.
         */
        auto idx = m_ids._Find_first();
        if (idx == N) {
            return (std::nullopt);
        }
        m_ids.reset(idx);
        return (std::make_optional(idx + m_offset));
    }

    void release(T x)
    {
        if (m_offset > x || static_cast<size_t>(x - m_offset) > N) {
            return;
        }

        x -= m_offset;
        assert(m_ids.test(x) == false);
        m_ids.set(x);
    }
};

}
}

#endif /* _ICP_SOCKET_ID_ALLOCATOR_H_ */
