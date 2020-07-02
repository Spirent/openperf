#include <cassert>
#include <numeric>

#include "packetio/drivers/dpdk/mbuf_rx_prbs.hpp"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/port/prbs_error_detector.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "spirent_pga/api.h"
#include "utils/prefetch_for_each.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::dpdk::port {

static constexpr auto chunk_size = 32U;
static constexpr auto prefetch_lookahead = 8U;

template <typename T> using chunk_array = std::array<T, chunk_size>;

static unsigned mbuf_segment_count(const rte_mbuf* m) { return (m->nb_segs); }

static uint16_t packet_count_for_segment_limit(rte_mbuf* const packets[],
                                               uint16_t nb_packets,
                                               uint16_t segment_limit)
{
    auto total_segs = 0U;
    auto cursor =
        std::find_if(packets, packets + nb_packets, [&](const auto& mbuf) {
            auto nb_segments = mbuf_segment_count(mbuf);
            if (total_segs + nb_segments > segment_limit) { return (true); }
            total_segs += nb_segments;
            return (false);
        });

    return (std::distance(packets, cursor));
}

static bool has_prbs_payload(const rte_mbuf* m)
{
    return (mbuf_signature_avail(m)
            && pga_prbs_flag(mbuf_signature_flags_get(m))
                   == pga_signature_prbs::enable);
}

static uint16_t get_payload_offset(const rte_mbuf* m)
{
    auto hdr_lens = rte_net_hdr_lens{};
    const auto ptype = rte_net_get_ptype(m, &hdr_lens, RTE_PTYPE_ALL_MASK);

    if (ptype & RTE_PTYPE_L2_ETHER_MPLS) {
        /*
         * XXX: DPDK's get_type function doesn't parse MPLS packets, so pick
         * some value that should skip over any headers that might be present
         * and into the payload. But don't pick too big!
         */
        return (std::min(static_cast<uint16_t>(RTE_PKTMBUF_HEADROOM),
                         rte_pktmbuf_data_len(m)));
    }

    return (hdr_lens.l2_len + hdr_lens.l3_len + hdr_lens.l4_len
            + hdr_lens.tunnel_len + hdr_lens.inner_l2_len
            + hdr_lens.inner_l3_len + hdr_lens.inner_l4_len);
}

/*
 * The Spirent signature occupies the last 20 bytes of payload,
 * so we may need to subtract that length from the segment if
 * it's the last one.
 */
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

static uint16_t detect_prbs_errors([[maybe_unused]] uint16_t port_id,
                                   [[maybe_unused]] uint16_t queue_id,
                                   rte_mbuf* packets[],
                                   uint16_t nb_packets,
                                   [[maybe_unused]] uint16_t max_packets,
                                   [[maybe_unused]] void* user_param)
{
    using prbs_container = openperf::utils::soa_container<
        chunk_array,
        std::tuple<const uint8_t*, uint16_t, uint32_t>>;

    auto prbs_packets = chunk_array<rte_mbuf*>{};
    auto prbs_segments = prbs_container{};

    auto start = 0U;
    while (start < nb_packets) {
        auto end = start
                   + packet_count_for_segment_limit(
                       packets + start, nb_packets - start, chunk_size);

        /* Copy all of the packets we need to check into a consecutive block */
        const auto prbs_end = std::copy_if(
            packets + start,
            packets + end,
            prbs_packets.data(),
            [](const auto* mbuf) { return (has_prbs_payload(mbuf)); });
        const auto nb_prbs_pkts = std::distance(prbs_packets.data(), prbs_end);

        /*
         * Find all the payload data in the PRBS packets we found. Since we
         * need to parse headers, perform pre-fetching as we go to minimize our
         * stalling on the packet data.
         */
        auto nb_prbs_segs = 0U;
        openperf::utils::prefetch_for_each(
            prbs_packets.data(),
            prbs_packets.data() + nb_prbs_pkts,
            [](const auto* mbuf) {
                rte_prefetch0(rte_pktmbuf_mtod(mbuf, void*));
            },
            [&](auto* mbuf) {
                /*
                 * We need to find the payload offset for the first
                 * segment.
                 */
                const auto offset = get_payload_offset(mbuf);

                prbs_segments.set(
                    nb_prbs_segs++,
                    {rte_pktmbuf_mtod_offset(mbuf, const uint8_t*, offset),
                     get_payload_length(mbuf, offset),
                     0});

                /*
                 * If there are any subsequent segments, they shouldn't
                 * have any offsets to worry about.
                 */
                while (mbuf->next != nullptr) {
                    mbuf = mbuf->next;
                    prbs_segments.set(nb_prbs_segs++,
                                      {rte_pktmbuf_mtod(mbuf, const uint8_t*),
                                       get_payload_length(mbuf),
                                       0});
                }
            },
            prefetch_lookahead);

        /* Now check the prbs data */
        pga_verify_prbs(prbs_segments.data<0>(),
                        prbs_segments.data<1>(),
                        nb_prbs_segs,
                        prbs_segments.data<2>());

        /* And update the packet metadata. */
        auto seg_idx = 0U;
        const auto& lengths = prbs_segments.data<1>();
        const auto& errors = prbs_segments.data<2>();
        std::for_each(
            prbs_packets.data(),
            prbs_packets.data() + nb_prbs_pkts,
            [&](auto* mbuf) {
                auto nb_segments = mbuf_segment_count(mbuf);
                auto bit_errors = std::accumulate(
                    &errors[seg_idx], &errors[seg_idx] + nb_segments, 0U);
                auto octets = std::accumulate(
                    &lengths[seg_idx], &lengths[seg_idx] + nb_segments, 0U);
                mbuf_rx_prbs_set(mbuf, octets, bit_errors);
                seg_idx += nb_segments;
            });

        start = end;
    }

    return (nb_packets);
}

std::string callback_prbs_error_detector::description()
{
    return ("Spirent PRBS error detecting");
}

rx_callback<callback_prbs_error_detector>::rx_callback_fn
callback_prbs_error_detector::callback()
{
    return (detect_prbs_errors);
}

static prbs_error_detector::variant_type
make_prbs_error_detector(uint16_t port_id)
{
    return (callback_prbs_error_detector(port_id));
}

prbs_error_detector::prbs_error_detector(uint16_t port_id)
    : feature(make_prbs_error_detector(port_id))
{
    pga_init();
}

} // namespace openperf::packetio::dpdk::port
