#include <array>

#include "lib/packet/protocol/protocols.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/port/checksum_calculator.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "spirent_pga/api.h"

namespace openperf::packetio::dpdk::port {

template <typename T> T get_next_header(libpacket::protocol::ipv4* ipv4)
{
    static_assert(std::is_pointer_v<T>);
    auto* cursor = reinterpret_cast<uint8_t*>(ipv4);
    cursor += get_ipv4_header_length(*ipv4);
    return (reinterpret_cast<T>(cursor));
}

template <typename T> T get_l3_header(struct rte_mbuf* m)
{
    static_assert(std::is_pointer_v<T>);
    return (rte_pktmbuf_mtod_offset(m, T, m->l2_len));
}

template <typename T> T get_l4_header(struct rte_mbuf* m)
{
    static_assert(std::is_pointer_v<T>);
    return (rte_pktmbuf_mtod_offset(m, T, m->l2_len + m->l3_len));
}

static uint16_t calculate_checksums([[maybe_unused]] uint16_t port_id,
                                    uint16_t queue_id,
                                    rte_mbuf* packets[],
                                    uint16_t nb_packets,
                                    void* user_param)
{
    using namespace libpacket::protocol;

    constexpr auto PKT_TX_L4_CKSUM = PKT_TX_TCP_CKSUM | PKT_TX_UDP_CKSUM;

    auto& scratch = reinterpret_cast<callback_checksum_calculator::scratch_t*>(
        user_param)[queue_id];

    auto start = 0U;

    while (start < nb_packets) {
        const auto end =
            start + std::min(utils::chunk_size, nb_packets - start);
        auto nb_ipv4_headers = 0U, nb_ipv4_tcpudp_headers = 0U,
             nb_ipv6_tcpudp_headers = 0U;

        /*
         * Use packet metadata to categorize packets. We need to know what
         * type of checksums to calculate.
         */
        std::for_each(packets + start, packets + end, [&](auto* mbuf) {
            if (mbuf->ol_flags & PKT_TX_IPV4) {
                auto* ipv4 = get_l3_header<struct ipv4*>(mbuf);
                scratch.ipv4_cksums.set(nb_ipv4_headers++, {ipv4, 0});

                if (mbuf->ol_flags & PKT_TX_L4_CKSUM) {
                    scratch.ipv4_tcpudp_cksums.set(nb_ipv4_tcpudp_headers++,
                                                   {ipv4, 0});

                    /*
                     * Note: the generator helpfully sets the pseudo-header
                     * value into the payload's checksum field. We need to clear
                     * it in order to calculate the correct checksum with our
                     * pga functions.
                     */
                    switch (mbuf->ol_flags & PKT_TX_L4_CKSUM) {
                    case PKT_TX_TCP_CKSUM: {
                        auto* tcp = get_l4_header<struct tcp*>(mbuf);
                        set_tcp_checksum(*tcp, 0);
                        break;
                    }
                    case PKT_TX_UDP_CKSUM: {
                        auto* udp = get_l4_header<struct udp*>(mbuf);
                        set_udp_checksum(*udp, 0);
                        break;
                    }
                    default:
                        break;
                    }
                }
            } else if (mbuf->ol_flags & PKT_TX_IPV6) {
                auto* ipv6 = get_l3_header<struct ipv6*>(mbuf);
                switch (mbuf->ol_flags & PKT_TX_L4_CKSUM) {
                case PKT_TX_TCP_CKSUM: {
                    auto* tcp = get_l4_header<struct tcp*>(mbuf);
                    set_tcp_checksum(*tcp, 0);
                    scratch.ipv6_tcpudp_cksums.set(
                        nb_ipv6_tcpudp_headers++,
                        {ipv6,
                         reinterpret_cast<uint8_t*>(tcp),
                         IPPROTO_TCP,
                         0});
                    break;
                }
                case PKT_TX_UDP_CKSUM: {
                    auto* udp = get_l4_header<struct udp*>(mbuf);
                    set_udp_checksum(*udp, 0);
                    scratch.ipv6_tcpudp_cksums.set(
                        nb_ipv6_tcpudp_headers++,
                        {ipv6,
                         reinterpret_cast<uint8_t*>(udp),
                         IPPROTO_UDP,
                         0});
                    break;
                }
                default:
                    break;
                }
            }
        });

        /* Now, calculate and fill in checksum data */
        if (nb_ipv4_headers) {
            pga_checksum_ipv4_headers(
                reinterpret_cast<uint8_t**>(scratch.ipv4_cksums.data<0>()),
                nb_ipv4_headers,
                scratch.ipv4_cksums.data<1>());

            if (nb_ipv4_tcpudp_headers) {
                pga_checksum_ipv4_tcpudp(
                    reinterpret_cast<uint8_t**>(
                        scratch.ipv4_tcpudp_cksums.data<0>()),
                    nb_ipv4_tcpudp_headers,
                    scratch.ipv4_tcpudp_cksums.data<1>());

                auto start = std::begin(scratch.ipv4_tcpudp_cksums);
                std::for_each(start,
                              std::next(start, nb_ipv4_tcpudp_headers),
                              [](auto&& tuple) {
                                  auto* ipv4 = std::get<0>(tuple);
                                  auto cksum = std::get<1>(tuple);
                                  switch (get_ipv4_protocol(*ipv4)) {
                                  case IPPROTO_TCP: {
                                      auto* tcp =
                                          get_next_header<struct tcp*>(ipv4);
                                      set_tcp_checksum(*tcp, htons(cksum));
                                      break;
                                  }
                                  case IPPROTO_UDP: {
                                      auto* udp =
                                          get_next_header<struct udp*>(ipv4);
                                      set_udp_checksum(*udp, htons(cksum));
                                      break;
                                  }
                                  default:
                                      break;
                                  }
                              });
            }

            auto start = std::begin(scratch.ipv4_cksums);
            std::for_each(
                start, std::next(start, nb_ipv4_headers), [](auto&& tuple) {
                    auto* ipv4 = std::get<0>(tuple);
                    auto cksum = std::get<1>(tuple);
                    set_ipv4_checksum(*ipv4, htons(cksum));
                });
        }

        if (nb_ipv6_tcpudp_headers) {
            pga_checksum_ipv6_tcpudp(reinterpret_cast<uint8_t**>(
                                         scratch.ipv6_tcpudp_cksums.data<0>()),
                                     scratch.ipv6_tcpudp_cksums.data<1>(),
                                     nb_ipv6_tcpudp_headers,
                                     scratch.ipv6_tcpudp_cksums.data<3>());

            auto start = std::begin(scratch.ipv6_tcpudp_cksums);
            std::for_each(start,
                          std::next(start, nb_ipv6_tcpudp_headers),
                          [](auto&& tuple) {
                              auto* payload = std::get<1>(tuple);
                              auto protocol = std::get<2>(tuple);
                              auto cksum = std::get<3>(tuple);
                              switch (protocol) {
                              case IPPROTO_TCP: {
                                  auto* tcp =
                                      reinterpret_cast<struct tcp*>(payload);
                                  set_tcp_checksum(*tcp, htons(cksum));
                                  break;
                              }
                              case IPPROTO_UDP: {
                                  auto* udp =
                                      reinterpret_cast<struct udp*>(payload);
                                  set_udp_checksum(*udp, htons(cksum));
                                  break;
                              }
                              default:
                                  break;
                              }
                          });
        }

        start = end;
    }

    return (nb_packets);
}

std::string callback_checksum_calculator::description()
{
    return ("IP checksum calculator");
}

tx_callback<callback_checksum_calculator>::tx_callback_fn
callback_checksum_calculator::callback()
{
    return (calculate_checksums);
}

void* callback_checksum_calculator::callback_arg() const
{
    if (scratch.size() != port_info::tx_queue_count(port_id())) {
        scratch.resize(port_info::tx_queue_count(port_id()));
    }
    return (scratch.data());
}

static checksum_calculator::variant_type
make_checksum_calculator(uint16_t port_id)
{
    constexpr auto tx_cksum_offloads =
        (DEV_TX_OFFLOAD_IPV4_CKSUM | DEV_TX_OFFLOAD_TCP_CKSUM
         | DEV_TX_OFFLOAD_UDP_CKSUM);

    if ((port_info::tx_offloads(port_id) & tx_cksum_offloads)
        != tx_cksum_offloads) {
        /* We need checksums! */
        return (callback_checksum_calculator(port_id));
    }

    return (null_feature(port_id));
}

checksum_calculator::checksum_calculator(uint16_t port_id)
    : feature(make_checksum_calculator(port_id))
{
    pga_init();
}

} // namespace openperf::packetio::dpdk::port
