#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/port/signature_decoder.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "spirent_pga/api.h"
#include "utils/prefetch_for_each.hpp"
#include "utils/soa_container.hpp"

#include "timesync/chrono.hpp"

namespace openperf::packetio::dpdk::port {

inline int64_t to_nanoseconds(const utils::phxtime timestamp,
                              const utils::phxtime offset)
{
    return (
        std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp + offset)
            .count());
}

/*
 * The timestamp in the Spirent signature field is 38 bits long. So, we mask
 * out the lower 32 bits when we calculate the offset to wall-clock time. Those
 * lower bits come from the timestamp directly.
 */
template <typename Clock>
inline utils::phxtime
get_phxtime_offset(const std::chrono::seconds epoch_offset)
{
    static constexpr uint64_t offset_mask = 0xffffffc000000000;
    auto now = Clock::now();

    auto phx_offset = std::chrono::duration_cast<utils::phxtime>(
        now.time_since_epoch() - epoch_offset);
    auto phx_fudge = utils::phxtime{phx_offset.count() & offset_mask};
    return (epoch_offset + phx_fudge);
}

static uint16_t detect_signatures([[maybe_unused]] uint16_t port_id,
                                  uint16_t queue_id,
                                  rte_mbuf* packets[],
                                  uint16_t nb_packets,
                                  [[maybe_unused]] uint16_t max_packets,
                                  void* user_param)
{
    auto& scratch = reinterpret_cast<callback_signature_decoder::scratch_t*>(
        user_param)[queue_id];

    /*
     * Given the epoch offset, find the offset that will shift the 38 bit,
     * 2.5 ns timestamp field to the current wall clock time, in nanoseconds.
     * The epoch_offset should be the number of seconds since the beginning
     * of the current year.
     */
    using clock = openperf::timesync::chrono::realtime;
    auto offset =
        get_phxtime_offset<clock>(std::chrono::seconds{scratch.epoch_offset});

    auto start = 0U;
    while (start < nb_packets) {
        auto end = start + std::min(utils::chunk_size, nb_packets - start);
        auto count = end - start;

        /*
         * The spirent signature should start 20 bytes before the end of the
         * packet, so generate an array of expected locations to check.
         */
        std::transform(
            packets + start,
            packets + end,
            scratch.payloads.data<1>(), /* storage */
            scratch.payloads.data<0>(), /* pointer */
            [&](const auto& mbuf, auto& storage) {
                return (static_cast<const uint8_t*>(rte_pktmbuf_read(
                    mbuf,
                    rte_pktmbuf_pkt_len(mbuf) - utils::signature_length,
                    utils::signature_length,
                    storage)));
            });

        /* Look for signature candidates */
        if (pga_signatures_crc_filter(scratch.payloads.data<0>(),
                                      count,
                                      scratch.crc_matches.data())) {

            /* Matches found; decode signatures */
            pga_signatures_decode(
                scratch.payloads.data<0>(),
                count,
                scratch.signatures.data<0>(),  /* stream id */
                scratch.signatures.data<1>(),  /* sequence num */
                scratch.signatures.data<2>(),  /* timestamp */
                scratch.signatures.data<3>()); /* flags */

            /*
             * Write valid signature data to the associated mbuf.
             * Since the 2nd half of the mbuf is unlikely to be in the cache
             * on platforms with 64 byte cache lines, we need to prefetch it
             * to avoid write stalls.
             */
            openperf::utils::prefetch_enumerate_for_each(
                packets + start,
                packets + start + count,
                [](const auto* mbuf) {
                    if constexpr (RTE_CACHE_LINE_SIZE == 64) {
                        __builtin_prefetch(mbuf->cacheline1, 1, 0);
                    }
                },
                [&](auto idx, auto* mbuf) {
                    const auto& sig = scratch.signatures[idx];
                    if (scratch.crc_matches[idx]
                        && (pga_status_flag(std::get<3>(sig))
                            == pga_signature_status::valid)) {
                        mbuf_signature_rx_set(
                            mbuf,
                            std::get<0>(sig),
                            std::get<1>(sig),
                            to_nanoseconds(utils::phxtime{std::get<2>(sig)},
                                           offset),
                            std::get<3>(sig));
                    }
                },
                mbuf_prefetch_offset);
        }

        start = end;
    }

    return (nb_packets);
}

std::string callback_signature_decoder::description()
{
    return ("Spirent signature decoding");
}

rx_callback<callback_signature_decoder>::rx_callback_fn
callback_signature_decoder::callback()
{
    return (detect_signatures);
}

void* callback_signature_decoder::callback_arg() const
{
    if (scratch.size() != port_info::rx_queue_count(port_id())) {
        scratch.resize(port_info::rx_queue_count(port_id()));
        auto offset = utils::get_timestamp_epoch_offset();
        for (auto&& item : scratch) { item.epoch_offset = offset; }
    }

    return (scratch.data());
}

static signature_decoder::variant_type make_signature_decoder(uint16_t port_id)
{
    return (callback_signature_decoder(port_id));
}

signature_decoder::signature_decoder(uint16_t port_id)
    : feature(make_signature_decoder(port_id))
{
    pga_init();
}

} // namespace openperf::packetio::dpdk::port
