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
        prbs,
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
            uint32_t unused : 6;
            uint32_t fill : 8;
            uint32_t offset : 16;
            union
            {
                uint8_t u8;
                uint16_t u16;
            } value;
        } tx;
    };

    static constexpr auto timestamp_mask = 0x3fffffffffffffffULL;
};

static_assert(sizeof(mbuf_signature) == 16);

constexpr auto mbuf_dynfield_signature =
    rte_mbuf_dynfield{.name = "packetio_dynfield_signature",
                      .size = sizeof(mbuf_signature),
                      .align = __alignof(uint64_t),
                      .flags = 0};

constexpr auto mbuf_dynflag_signature =
    rte_mbuf_dynflag{.name = "packetio_dynflag_signature", .flags = 0};

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

template <typename Enum>
std::underlying_type_t<Enum> to_underlying_type(const Enum& e)
{
    return (static_cast<std::underlying_type_t<Enum>>(e));
}

inline void mbuf_signature_tx_set_fill_const(rte_mbuf* mbuf,
                                             uint16_t offset,
                                             uint16_t value)
{
    auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    mbuf_sig->tx.fill = to_underlying_type(mbuf_signature::fill_type::constant);
    mbuf_sig->tx.offset = offset;
    mbuf_sig->tx.value.u16 = value;
}

inline void
mbuf_signature_tx_set_fill_decr(rte_mbuf* mbuf, uint16_t offset, uint8_t value)
{
    auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    mbuf_sig->tx.fill =
        to_underlying_type(mbuf_signature::fill_type::decrement);
    mbuf_sig->tx.offset = offset;
    mbuf_sig->tx.value.u8 = value;
}

inline void
mbuf_signature_tx_set_fill_incr(rte_mbuf* mbuf, uint16_t offset, uint8_t value)
{
    auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    mbuf_sig->tx.fill =
        to_underlying_type(mbuf_signature::fill_type::increment);
    mbuf_sig->tx.offset = offset;
    mbuf_sig->tx.value.u8 = value;
}

inline void mbuf_signature_tx_set_fill_prbs(rte_mbuf* mbuf, uint16_t offset)
{
    auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    mbuf_sig->tx.fill = to_underlying_type(mbuf_signature::fill_type::prbs);
    mbuf_sig->tx.offset = offset;
}

inline mbuf_signature::fill_type
mbuf_signature_tx_get_fill_type(const rte_mbuf* mbuf)
{
    const auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    return (static_cast<mbuf_signature::fill_type>(mbuf_sig->tx.fill));
}

inline uint16_t mbuf_signature_tx_get_fill_offset(const rte_mbuf* mbuf)
{
    const auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    return (mbuf_sig->tx.offset);
}

inline uint16_t mbuf_signature_tx_get_fill_const(const rte_mbuf* mbuf)
{
    const auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    return (mbuf_sig->tx.value.u16);
}

inline uint8_t mbuf_signature_tx_get_fill_incr(const rte_mbuf* mbuf)
{
    const auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    return (mbuf_sig->tx.value.u8);
}

inline uint8_t mbuf_signature_tx_get_fill_decr(const rte_mbuf* mbuf)
{
    const auto* mbuf_sig =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_signature_offset, mbuf_signature*);
    return (mbuf_sig->tx.value.u8);
}

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
            .rx = {.flags = flags,
                   .timestamp = timestamp & mbuf_signature::timestamp_mask}};
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
