#ifndef _OP_PACKETIO_DPDK_MBUF_METADATA_HPP_
#define _OP_PACKETIO_DPDK_MBUF_METADATA_HPP_

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "rte_ether.h"
#include "rte_mbuf.h"
#include "rte_mbuf_dyn.h"

namespace openperf::packetio::dpdk {

struct prbs_data
{
    uint32_t octets;
    uint32_t bit_errors;
};

struct mbuf_signature
{
    uint32_t stream_id;
    uint32_t sequence_number;

    enum class fill_type : uint8_t {
        none = 0,
        constant,
        increment,
        decrement,
        prbs,
    };

    static constexpr auto timestamp_mask = 0x3fffffffffffffffULL;

    union
    {
        struct
        {
            /*
             * This 62 bit timestamp contains the wall-clock time
             * in nanoseconds since the start of the UNIX epoch,
             * e.g. 1970. It will roll over sometime in 2116.
             */
            uint64_t flags : 2;
            uint64_t timestamp : 62;
            prbs_data prbs;
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
};

struct mbuf_metadata
{
    std::chrono::nanoseconds timestamp;
    union
    {
        struct mbuf_signature signature;
        struct
        {
            uint8_t hwaddr[RTE_ETHER_ADDR_LEN];
        } tx_sink;
        void* tag;
    };
};

extern size_t mbuf_metadata_offset;
extern uint64_t mbuf_prbs_data_flag; /* Rx PRBS decode data is set */
extern uint64_t mbuf_signature_flag; /* signature data is set */
extern uint64_t mbuf_tagged_flag;    /* interface has a tag attached */
extern uint64_t mbuf_timestamp_flag; /* timestamp is set */
extern uint64_t mbuf_tx_sink_flag;   /* tx sink data is set */

void mbuf_metadata_init();

inline void mbuf_metadata_clear(rte_mbuf* mbuf)
{
    mbuf->ol_flags &=
        ~(mbuf_prbs_data_flag | mbuf_signature_flag | mbuf_tagged_flag
          | mbuf_timestamp_flag | mbuf_tx_sink_flag);
}

/**
 * Timestamp metadata functions
 */

inline bool mbuf_timestamp_is_set(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_timestamp_flag);
}

template <typename Duration>
inline void mbuf_timestamp_set(rte_mbuf* mbuf, const Duration& timestamp)
{
    mbuf->ol_flags |= mbuf_timestamp_flag;
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->timestamp =
        std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp);
}

inline std::chrono::nanoseconds mbuf_timestamp_get(const rte_mbuf* mbuf)
{
    assert(mbuf_timestamp_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (meta->timestamp);
}

/**
 * Signature metadata functions
 */

inline bool mbuf_signature_is_set(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_signature_flag);
}

inline void mbuf_signature_tx_set(rte_mbuf* mbuf,
                                  uint32_t stream_id,
                                  uint32_t seq_num,
                                  uint32_t flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature = mbuf_signature{
        .stream_id = stream_id, .sequence_number = seq_num, .tx.flags = flags};
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
    assert(mbuf_signature_is_set(mbuf));
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature.tx.fill =
        to_underlying_type(mbuf_signature::fill_type::constant);
    meta->signature.tx.offset = offset;
    meta->signature.tx.value.u16 = value;
}

inline void
mbuf_signature_tx_set_fill_decr(rte_mbuf* mbuf, uint16_t offset, uint8_t value)
{
    assert(mbuf_signature_is_set(mbuf));
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature.tx.fill =
        to_underlying_type(mbuf_signature::fill_type::decrement);
    meta->signature.tx.offset = offset;
    meta->signature.tx.value.u8 = value;
}

inline void
mbuf_signature_tx_set_fill_incr(rte_mbuf* mbuf, uint16_t offset, uint8_t value)
{
    assert(mbuf_signature_is_set(mbuf));
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature.tx.fill =
        to_underlying_type(mbuf_signature::fill_type::increment);
    meta->signature.tx.offset = offset;
    meta->signature.tx.value.u8 = value;
}

inline void mbuf_signature_tx_set_fill_prbs(rte_mbuf* mbuf, uint16_t offset)
{
    assert(mbuf_signature_is_set(mbuf));
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature.tx.fill =
        to_underlying_type(mbuf_signature::fill_type::prbs);
    meta->signature.tx.offset = offset;
}

inline mbuf_signature::fill_type
mbuf_signature_tx_get_fill_type(const rte_mbuf* mbuf)
{
    assert(mbuf_signature_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (static_cast<mbuf_signature::fill_type>(meta->signature.tx.fill));
}

inline uint16_t mbuf_signature_tx_get_fill_offset(const rte_mbuf* mbuf)
{
    assert(mbuf_signature_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (meta->signature.tx.offset);
}

inline uint16_t mbuf_signature_tx_get_fill_const(const rte_mbuf* mbuf)
{
    assert(mbuf_signature_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (meta->signature.tx.value.u16);
}

inline uint8_t mbuf_signature_tx_get_fill_incr(const rte_mbuf* mbuf)
{
    assert(mbuf_signature_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (meta->signature.tx.value.u8);
}

inline uint8_t mbuf_signature_tx_get_fill_decr(const rte_mbuf* mbuf)
{
    assert(mbuf_signature_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (meta->signature.tx.value.u8);
}

inline void mbuf_signature_rx_set(rte_mbuf* mbuf,
                                  uint32_t stream_id,
                                  uint32_t seq_num,
                                  uint64_t timestamp,
                                  uint32_t flags)
{
    mbuf->ol_flags |= mbuf_signature_flag;
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature = mbuf_signature{
        .stream_id = stream_id,
        .sequence_number = seq_num,
        .rx = {.flags = flags,
               .timestamp = timestamp & mbuf_signature::timestamp_mask}};
}

inline const mbuf_signature& mbuf_signature_get(const rte_mbuf* mbuf)
{
    assert(mbuf_signature_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, const mbuf_metadata*);
    return (meta->signature);
}

inline uint32_t mbuf_signature_get_stream_id(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).stream_id);
}

inline uint32_t mbuf_signature_get_seq_num(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).sequence_number);
}

inline uint64_t mbuf_signature_get_timestamp(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).rx.timestamp);
}

inline int mbuf_signature_get_flags(const rte_mbuf* mbuf)
{
    return (mbuf_signature_get(mbuf).rx.flags);
}

/**
 * Transmit Sink metadata functions
 */

/** Get mbuf ol_flag indicating mbuf should be pased to tx sink */
inline bool mbuf_tx_sink_is_set(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_tx_sink_flag);
}

/**
 * Set mbuf ol_flag indicating the mbuf should be passed to tx sink
 * and store the hardware address of the interface used for transmit
 * in the mbuf.
 *
 * The hardware address is stored in mbuf dynamic data and should be
 * considered transient. It is only valid in certain contexts, hence
 * the explicit flag.
 */
inline void mbuf_tx_sink_set(rte_mbuf* mbuf, const uint8_t hwaddr[])
{
    // assert(!(mbuf_tx_sink_is_set(mbuf)));
    mbuf->ol_flags |= mbuf_tx_sink_flag;
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    std::copy_n(hwaddr, RTE_ETHER_ADDR_LEN, meta->tx_sink.hwaddr);
}

/** Clear mbuf ol_flag indicating the mbuf should be passed to tx sink.
 */
inline void mbuf_tx_sink_clear(rte_mbuf* mbuf)
{
    mbuf->ol_flags &= (~mbuf_tx_sink_flag);
}

/**
 * Get the the interface hardware address used for transmit.
 *
 * The hardware address is stored in the mbuf user data and should be considered
 * transient.  It is only valid in certain parts of the transmit path.
 */
inline void mbuf_tx_sink_get_hwaddr(const rte_mbuf* mbuf, uint8_t hwaddr[])
{
    assert(mbuf_tx_sink_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    std::copy_n(meta->tx_sink.hwaddr, RTE_ETHER_ADDR_LEN, hwaddr);
}

/**
 * Receive PRBS metadata functions
 */

inline bool mbuf_rx_prbs_is_set(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_prbs_data_flag);
}

inline void
mbuf_rx_prbs_set(rte_mbuf* mbuf, uint32_t octets, uint32_t bit_errors)
{
    assert(mbuf_signature_is_set(mbuf));
    mbuf->ol_flags |= mbuf_prbs_data_flag;
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->signature.rx.prbs =
        prbs_data{.octets = octets, .bit_errors = bit_errors};
}

inline uint32_t mbuf_rx_prbs_get_octets(const rte_mbuf* mbuf)
{
    assert(mbuf_rx_prbs_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    return (meta->signature.rx.prbs.octets);
}

inline uint32_t mbuf_rx_prbs_get_bit_errors(const rte_mbuf* mbuf)
{
    assert(mbuf_rx_prbs_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    return (meta->signature.rx.prbs.bit_errors);
}

/**
 * Tag pointer metadata functions
 */

inline bool mbuf_tag_is_set(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_tagged_flag);
}

inline void mbuf_tag_clear(rte_mbuf* mbuf)
{
    mbuf->ol_flags &= ~mbuf_tagged_flag;
}

template <typename T> inline void mbuf_tag_set(rte_mbuf* mbuf, T* tag)
{
    assert(!mbuf_signature_is_set(mbuf));
    assert(!mbuf_tx_sink_is_set(mbuf));
    mbuf->ol_flags |= mbuf_tagged_flag;
    auto* meta = RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    meta->tag = const_cast<std::remove_const_t<T>*>(tag);
}

template <typename T> T* mbuf_tag_get(const rte_mbuf* mbuf)
{
    assert(mbuf_tag_is_set(mbuf));
    const auto* meta =
        RTE_MBUF_DYNFIELD(mbuf, mbuf_metadata_offset, mbuf_metadata*);
    return (static_cast<T*>(meta->tag));
}

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_MBUF_METADATA_HPP_ */
