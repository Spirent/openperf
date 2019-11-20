#ifndef _ICP_PACKETIO_STACK_ATOMIC_SNMP_MACROS_H_
#define _ICP_PACKETIO_STACK_ATOMIC_SNMP_MACROS_H_

/*
 * Atomic MIB2 macros for LwIP.
 * We currently only need them for C++ code, hence we only have C++ versions.
 */

#ifdef __cplusplus
#include <atomic>

#define MIB2_STATS_NETIF_ADD_ATOMIC(n, x, val)                          \
    do {                                                                \
        using counter_type = decltype(n->mib2_counters.x);              \
        std::atomic_fetch_add_explicit(                                 \
            reinterpret_cast<std::atomic<counter_type>*>(               \
                std::addressof(n->mib2_counters.x)),                    \
            static_cast<counter_type>(val),                             \
            std::memory_order_relaxed);                                 \
    } while (0)

#define MIB2_STATS_NETIF_INC_ATOMIC(n, x) MIB2_STATS_NETIF_ADD_ATOMIC(n, x, 1)

#endif /* __cplusplus */

#endif /* _ICP_PACKETIO_STACK_ATOMIC_SNMP_MACROS_H_ */
