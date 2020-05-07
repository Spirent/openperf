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

    enum class fill_type : uint8_t {
        none = 0,
        constant,
        increment,
        decrement,
        prbs
    };

    union
    {
        uint64_t sig_data;
        struct
        {
            /*
             * This 62 bit timestamp contains the wall-clock time
             * in nanoseconds since the start of the UNIX epoch,
             * e.g. 1970. It will roll over sometime in 2116.
             */
            uint64_t flags : 2;
            uint64_t timestamp : 62;
        } rx;
        struct
        {
            uint32_t flags : 2;
            uint32_t unused : 22;
            uint32_t fill : 8;
            uint32_t value;
        } tx;
    };

    static constexpr auto timestamp_mask = 0x3fffffffffffffffULL;
};

static_assert(sizeof(mbuf_signature) == 16);

extern size_t mbuf_signature_offset;
extern uint64_t mbuf_signature_flag;

void mbuf_signature_init();

inline void mbuf_signature_tx_set(rte_mbuf* mbuf,
                                  uint32_t stream_id,
                                  uint32_t seq_num,
                                  uint32_t flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{.sig_stream_id = stream_id,
                       .sig_seq_num = seq_num,
                       .tx.flags = flags};
}

#if 0
inline void mbuf_signature_tx_set_const(rte_mbuf* mbuf,
                                        uint32_t stream_id,
                                        uint32_t seq_num,
                                        uint16_t fill_value,
                                        int flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{.sig_stream_id = stream_id,
                       .sig_seq_num = seq_num,
                       .tx = {.fill = mbuf_signature::fill_type::constant,
                              .value = fill_value,
                              .flags = flags}};
}

inline void mbuf_signature_tx_set_decr(rte_mbuf* mbuf,
                                       uint32_t stream_id,
                                       uint32_t seq_num,
                                       uint8_t fill_value,
                                       int flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{.sig_stream_id = stream_id,
                       .sig_seq_num = seq_num,
                       .tx = {.fill = mbuf_signature::fill_type::decrement,
                              .value = fill_value,
                              .flags = flags}};
}

inline void mbuf_signature_tx_set_incr(rte_mbuf* mbuf,
                                       uint32_t stream_id,
                                       uint32_t seq_num,
                                       uint8_t fill_value,
                                       int flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{.sig_stream_id = stream_id,
                       .sig_seq_num = seq_num,
                       .tx = {.fill = mbuf_signature::fill_type::increment,
                              .value = fill_value,
                              .flags = flags}};
}

inline void mbuf_signature_tx_set_prbs(rte_mbuf* mbuf,
                                       uint32_t stream_id,
                                       uint32_t seq_num,
                                       uint32_t seed,
                                       int flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{.sig_stream_id = stream_id,
                       .sig_seq_num = seq_num,
                       .tx = {.fill = mbuf_signature::fill_type::prbs,
                              .value = seed,
                              .flags = flags}};
}
#endif

inline void mbuf_signature_rx_set(rte_mbuf* mbuf,
                                  uint32_t stream_id,
                                  uint32_t seq_num,
                                  uint64_t timestamp,
                                  uint32_t flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    *(RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*)) =
        mbuf_signature{
            .sig_stream_id = stream_id,
            .sig_seq_num = seq_num,
            .rx = {.timestamp = timestamp & mbuf_signature::timestamp_mask,
                   .flags = flags}};
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
    return (mbuf_signature_get(mbuf).rx.timestamp);
}

inline int mbuf_signature_flags_get(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).rx.flags);
}

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_MUBF_SIGNATURE_HPP_ */
