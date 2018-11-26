#include <cstddef>
#include <optional>

#include "lwip/def.h"

struct rte_ring;

namespace lwip {

template <typename T>
class system_mailbox
{
    static constexpr size_t mailbox_size = 128;
    rte_ring* m_ring;
    int m_fd;

public:
    system_mailbox();
    ~system_mailbox();

    void post(T* msg);
    void trypost(T* msg);

    std::optional<T*> fetch(u32_t timeout);
    std::optional<T*> tryfetch();
};

}
