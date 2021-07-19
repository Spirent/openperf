#include <cassert>

#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/port/signature_payload_filler.hpp"
#include "spirent_pga/api.h"

namespace openperf::packetio::dpdk::port {

static unsigned mbuf_segment_count(const rte_mbuf* m) { return (m->nb_segs); }

static uint16_t packet_segment_limit(rte_mbuf* const packets[],
                                     uint16_t nb_packets)
{
    auto const_segs = 0U, incr_segs = 0U, decr_segs = 0U, prbs_segs = 0U;
    auto cursor =
        std::find_if(packets, packets + nb_packets, [&](const auto* mbuf) {
            const auto nb_segs = mbuf_segment_count(mbuf);
            if (!mbuf_signature_avail(mbuf)) { return (false); }
            switch (mbuf_signature_tx_get_fill_type(mbuf)) {
            case mbuf_signature::fill_type::constant:
                const_segs += nb_segs;
                return (const_segs > utils::chunk_size);
            case mbuf_signature::fill_type::increment:
                incr_segs += nb_segs;
                return (incr_segs > utils::chunk_size);
            case mbuf_signature::fill_type::decrement:
                decr_segs += nb_segs;
                return (decr_segs > utils::chunk_size);
            case mbuf_signature::fill_type::prbs:
                prbs_segs += nb_segs;
                return (prbs_segs > utils::chunk_size);
            default:
                return (false);
            }
        });

    /* Note: cursor points to packet that would overflow chunk_size segments */
    return (cursor == packets + nb_packets /* or end */
                ? nb_packets
                : std::distance(packets, cursor) - 1);
}

static uint16_t get_payload_offset(const rte_mbuf* m)
{
    return (mbuf_signature_tx_get_fill_offset(m));
}

static uint16_t get_payload_length(const rte_mbuf* m)
{
    assert(rte_pktmbuf_data_len(m) >= utils::signature_length);
    return (m->next ? rte_pktmbuf_data_len(m)
                    : rte_pktmbuf_data_len(m) - utils::signature_length);
}

static uint16_t get_payload_length(const rte_mbuf* m, uint16_t payload_offset)
{
    assert(rte_pktmbuf_data_len(m) > payload_offset + utils::signature_length);
    return (m->next ? rte_pktmbuf_data_len(m) - payload_offset
                    : rte_pktmbuf_data_len(m) - payload_offset
                          - utils::signature_length);
}

template <typename Container, typename T>
unsigned set_container_args(
    Container& args, unsigned index, rte_mbuf* mbuf, uint16_t offset, T value)
{
    args.set(index++,
             {rte_pktmbuf_mtod_offset(mbuf, uint8_t*, offset),
              get_payload_length(mbuf, offset),
              value});

    while (mbuf->next != nullptr) {
        mbuf = mbuf->next;
        args.set(index++,
                 {rte_pktmbuf_mtod(mbuf, uint8_t*),
                  get_payload_length(mbuf),
                  value});
    }

    return (index);
}

static uint16_t fill_signature_payloads([[maybe_unused]] uint16_t port_id,
                                        uint16_t queue_id,
                                        rte_mbuf* packets[],
                                        uint16_t nb_packets,
                                        void* user_param)
{
    auto& scratch =
        reinterpret_cast<callback_signature_payload_filler::scratch_t*>(
            user_param)[queue_id];

    auto start = 0U;
    while (start < nb_packets) {
        auto end =
            start + packet_segment_limit(packets + start, nb_packets - start);

        auto const_segs = 0U, incr_segs = 0U, decr_segs = 0U, prbs_segs = 0U;
        /* Sort segments into contiguous blocks where we can blast data */
        std::for_each(packets + start, packets + end, [&](auto* mbuf) {
            if (!mbuf_signature_avail(mbuf)) { return; }

            const auto offset = get_payload_offset(mbuf);

            if (rte_pktmbuf_pkt_len(mbuf) <= offset + utils::signature_length) {
                /* No payload data to fill */
                return;
            }

            switch (mbuf_signature_tx_get_fill_type(mbuf)) {
            case mbuf_signature::fill_type::constant:
                const_segs =
                    set_container_args(scratch.const_args,
                                       const_segs,
                                       mbuf,
                                       offset,
                                       mbuf_signature_tx_get_fill_const(mbuf));
                break;
            case mbuf_signature::fill_type::increment:
                incr_segs =
                    set_container_args(scratch.incr_args,
                                       incr_segs,
                                       mbuf,
                                       offset,
                                       mbuf_signature_tx_get_fill_incr(mbuf));
                break;
            case mbuf_signature::fill_type::decrement:
                decr_segs =
                    set_container_args(scratch.decr_args,
                                       decr_segs,
                                       mbuf,
                                       offset,
                                       mbuf_signature_tx_get_fill_decr(mbuf));
                break;
            case mbuf_signature::fill_type::prbs:
                scratch.prbs_args.set(
                    prbs_segs++,
                    {rte_pktmbuf_mtod_offset(mbuf, uint8_t*, offset),
                     get_payload_length(mbuf, offset)});

                while (mbuf->next != nullptr) {
                    mbuf = mbuf->next;
                    scratch.prbs_args.set(prbs_segs++,
                                          {rte_pktmbuf_mtod(mbuf, uint8_t*),
                                           get_payload_length(mbuf)});
                }
                break;
            default:
                /* nooop */
                break;
            }
        });

        if (const_segs) {
            pga_fill_const(scratch.const_args.data<0>(),
                           scratch.const_args.data<1>(),
                           const_segs,
                           scratch.const_args.data<2>());
        }

        if (incr_segs) {
            pga_fill_incr(scratch.incr_args.data<0>(),
                          scratch.incr_args.data<1>(),
                          incr_segs,
                          scratch.incr_args.data<2>());
        }

        if (decr_segs) {
            pga_fill_decr(scratch.decr_args.data<0>(),
                          scratch.decr_args.data<1>(),
                          decr_segs,
                          scratch.decr_args.data<2>());
        }

        if (prbs_segs) {
            scratch.prbs_seed = pga_fill_prbs(scratch.prbs_args.data<0>(),
                                              scratch.prbs_args.data<1>(),
                                              prbs_segs,
                                              scratch.prbs_seed);
        }

        start = end;
    }

    return (nb_packets);
}

std::string callback_signature_payload_filler::description()
{
    return ("Spirent signature payload fills");
}

tx_callback<callback_signature_payload_filler>::tx_callback_fn
callback_signature_payload_filler::callback()
{
    return (fill_signature_payloads);
}

void* callback_signature_payload_filler::callback_arg() const
{
    if (scratch.size() != port_info::tx_queue_count(port_id())) {
        scratch.resize(port_info::tx_queue_count(port_id()));
    }
    return (scratch.data());
}

static signature_payload_filler::variant_type
make_signature_payload_filler(uint16_t port_id)
{
    return (callback_signature_payload_filler(port_id));
}

signature_payload_filler::signature_payload_filler(uint16_t port_id)
    : feature(make_signature_payload_filler(port_id))
{
    pga_init();
}

} // namespace openperf::packetio::dpdk::port
