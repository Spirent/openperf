#ifndef _OP_PACKETIO_DPDK_MBUF_SIGNATURE_HPP_
#define _OP_PACKETIO_DPDK_MBUF_SIGNATURE_HPP_

#include <cstdint>
#include <cstddef>
#include <tuple>

#include "packetio/drivers/dpdk/dpdk.h"

namespace openperf::packetio::dpdk {

struct mbuf_signature
{
    uint32_t sig_stream_id;
    uint32_t sig_seq_num;

    /*
     * We use the upper 2 bits of the timestamp to store the flags.
     * The lower 62 bits contain the wall-clock time in nanoseconds
     * since the start of the UNIX epoch, e.g. 1970.  The 62 bit counter
     * will roll over sometime in 2116.
     */
    static constexpr auto flag_shift = 62U;
    static constexpr auto timestamp_mask = 0x3fffffffffffffffULL;
    uint64_t sig_timestamp_flags;
};

extern size_t mbuf_signature_offset;
extern uint64_t mbuf_signature_flag;

void mbuf_signature_init();

inline void mbuf_signature_set(rte_mbuf* mbuf,
                               uint32_t stream_id,
                               uint32_t seq_num,
                               uint64_t timestamp,
                               int flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{
            .sig_stream_id = stream_id,
            .sig_seq_num = seq_num,
            .sig_timestamp_flags =
                ((static_cast<uint64_t>(flags) << mbuf_signature::flag_shift)
                 | (timestamp & mbuf_signature::timestamp_mask))};
}

inline bool mbuf_signature_avail(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_signature_flag);
}

inline const mbuf_signature& mbuf_signature_get(const rte_mbuf* mbuf)
{
    return (*(
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, const mbuf_signature*)));
}

inline uint32_t mbuf_signature_stream_id_get(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).sig_stream_id);
}

inline uint32_t mbuf_signature_seq_num_get(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).sig_seq_num);
}

inline uint64_t mbuf_signature_timestamp_get(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).sig_timestamp_flags
            & mbuf_signature::timestamp_mask);
}

inline int mbuf_signature_flags_get(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).sig_timestamp_flags
            >> mbuf_signature::flag_shift);
}

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_MUBF_SIGNATURE_HPP_ */
