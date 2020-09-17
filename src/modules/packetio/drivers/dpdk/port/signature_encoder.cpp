#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/drivers/dpdk/port/signature_encoder.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "spirent_pga/api.h"
#include "units/data-rates.hpp"
#include "utils/soa_container.hpp"

#include "timesync/chrono.hpp"

namespace openperf::packetio::dpdk::port {

static constexpr auto ethernet_overhead = 24U;
static constexpr auto ethernet_octets = 8U;

/* sizeof(ipv4) + sizeof(udp) + sizeof(signature) */
static constexpr auto min_ipv4_udp_payload_size = 48U;

static bool is_runt_ipv4_udp(const rte_mbuf* mbuf)
{
    /*
     * Use the checksum flags as a proxy for packet contents. Obviously,
     * this doesn't work if sources don't set these flags...
     */
    static constexpr auto ol_flags = PKT_TX_IPV4 | PKT_TX_UDP_CKSUM;
    return ((mbuf->ol_flags & ol_flags) == ol_flags
            && (rte_pktmbuf_pkt_len(mbuf) - mbuf->l2_len)
                   < min_ipv4_udp_payload_size);
}

template <typename T> static T* to_signature(rte_mbuf* mbuf)
{
    return (rte_pktmbuf_mtod_offset(
        mbuf, T*, rte_pktmbuf_pkt_len(mbuf) - utils::signature_length));
}

static uint32_t get_link_speed_safe(uint16_t port_id)
{
    /* Query the port's link speed */
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);

    /*
     * Check the link status.  Sometimes, we get packets before
     * the port internals realize the link is up.  If that's the
     * case, then return the maximum speed of the port as a
     * safe default.
     */
    return (link.link_status == ETH_LINK_UP ? link.link_speed
                                            : port_info::max_speed(port_id));
}

using picoseconds = std::chrono::duration<int64_t, std::pico>;

inline int64_t to_phxtime(utils::phxtime now,
                          unsigned tx_octets,
                          const picoseconds ps_per_octet)
{
    return (
        (now
         + std::chrono::duration_cast<utils::phxtime>(tx_octets * ps_per_octet))
            .count());
}

/*
 * Calculate the current time in 2.5 ns ticks, aka phxtime. Since the signature
 * only contains rooms for 38 bits, we mask out everything else.
 */
template <typename Clock>
inline utils::phxtime get_phxtime(const std::chrono::seconds epoch_offset)
{
    static constexpr uint64_t phxtime_mask = 0x0000003fffffffff;
    auto now = Clock::now();

    auto phx_offset = std::chrono::duration_cast<utils::phxtime>(
        now.time_since_epoch() - epoch_offset);
    return (utils::phxtime{phx_offset.count() & phxtime_mask});
}

static uint16_t encode_signatures(uint16_t port_id,
                                  [[maybe_unused]] uint16_t queue_id,
                                  rte_mbuf* packets[],
                                  uint16_t nb_packets,
                                  void* user_param)
{
    /*
     * Since octets take less than 1 nanosecond as 100G speeds, we
     * calculate offsets in picoseconds.
     */
    using mbps = units::rate<uint64_t, units::megabits>;
    const auto ps_per_octet =
        units::to_duration<picoseconds>(mbps(get_link_speed_safe(port_id)))
        * ethernet_octets;

    auto* scratch =
        reinterpret_cast<callback_signature_encoder::scratch_t*>(user_param);

    /*
     * Given the epoch offset, find the current time as a 38 bit, 2.5ns
     * timestamp field.
     */
    using clock = openperf::timesync::chrono::realtime;
    const auto now =
        get_phxtime<clock>(std::chrono::seconds{scratch->epoch_offset});

    auto tx_octets = 0U;
    auto start = 0U;
    while (start < nb_packets) {
        const auto end =
            start + std::min(utils::chunk_size, nb_packets - start);

        /*
         * Look for packets that have signature data; they should have
         * the sig flag set in the offload flags. Stuff all of the
         * signature data into our SOA container so we can hand off
         * to the encoder.
         *
         * Use the total transmitted octet count to generate the proper
         * timestamp offset for the signature data. The signature timestamp
         * flag indicates whether to include the frames octet count or not.
         */
        auto nb_sigs = 0U;
        std::for_each(packets + start, packets + end, [&](const auto& m) {
            const auto mbuf_octets = rte_pktmbuf_pkt_len(m) + ethernet_overhead;

            if (mbuf_signature_avail(m)) {
                auto&& sig = mbuf_signature_get(m);
                const auto ts =
                    (pga_timestamp_flag(sig.tx.flags)
                             == pga_signature_timestamp::first
                         ? to_phxtime(now, tx_octets, ps_per_octet)
                         : to_phxtime(
                             now, tx_octets + mbuf_octets, ps_per_octet));

                scratch->signatures.set(nb_sigs++,
                                        {to_signature<uint8_t>(m),
                                         sig.sig_stream_id,
                                         sig.sig_seq_num,
                                         ts,
                                         sig.tx.flags});
            }
            tx_octets += mbuf_octets;
        });

        pga_signatures_encode(scratch->signatures.data<0>(), /* payload */
                              scratch->signatures.data<1>(), /* stream id */
                              scratch->signatures.data<2>(), /* sequence num */
                              scratch->signatures.data<3>(), /* timestamp */
                              scratch->signatures.data<4>(), /* flags */
                              nb_sigs);

        /*
         * When signatures are written to short IPv4/UDP packets, the signature
         * can overwrite the UDP checksum field. As a result, we need to disable
         * hardware UDP checksums and set the signature cheater field in its
         * place.
         */
        if (auto cursor = std::find_if(
                packets + start,
                packets + end,
                [](const auto& mbuf) { return (is_runt_ipv4_udp(mbuf)); });
            cursor != packets + end) {

            /* Generate the data we need in a compact format */
            auto nb_runts = 0U;
            std::for_each(cursor, packets + end, [&](const auto& mbuf) {
                if (is_runt_ipv4_udp(mbuf)) {
                    /* Clear UDP offload flag */
                    mbuf->ol_flags &= ~PKT_TX_UDP_CKSUM;

                    /*
                     * Store data for checksum calculating and
                     * updating.
                     */
                    scratch->runts.set(
                        nb_runts++,
                        {rte_pktmbuf_mtod_offset(
                             mbuf, const uint8_t*, mbuf->l2_len),
                         to_signature<utils::spirent_signature>(mbuf)});
                }
            });

            /* Then crunch the checksums */
            pga_checksum_ipv4_tcpudp(
                scratch->runts.data<0>(), nb_runts, scratch->checksums.data());

            /* And write them into the signature field */
            auto idx = 0U;
            std::for_each(scratch->runts.data<1>(),
                          scratch->runts.data<1>() + nb_runts,
                          [&](const auto& sig) {
                              sig->cheater = scratch->checksums[idx++];
                          });
        }

        start = end;
    }

    return (nb_packets);
}

std::string callback_signature_encoder::description()
{
    return ("Spirent signature encoding");
}

tx_callback<callback_signature_encoder>::tx_callback_fn
callback_signature_encoder::callback()
{
    return (encode_signatures);
}

void* callback_signature_encoder::callback_arg() const
{
    scratch.epoch_offset = utils::get_timestamp_epoch_offset();
    return (std::addressof(scratch));
}

static signature_encoder::variant_type make_signature_encoder(uint16_t port_id)
{
    return (callback_signature_encoder(port_id));
}

signature_encoder::signature_encoder(uint16_t port_id)
    : feature(make_signature_encoder(port_id))
{
    pga_init();
}

} // namespace openperf::packetio::dpdk::port
