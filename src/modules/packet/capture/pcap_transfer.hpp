#ifndef _OP_PACKET_CAPTURE_PCAP_TRANSFER_HPP_
#define _OP_PACKET_CAPTURE_PCAP_TRANSFER_HPP_

#include <pistache/http.h>

namespace openperf::packet::capture {
class transfer_context;
}

namespace openperf::packet::capture::pcap {

std::unique_ptr<transfer_context>
create_pcap_transfer_context(Pistache::Http::ResponseWriter& response,
                             uint64_t packet_start = 0,
                             uint64_t packet_end = UINT64_MAX);

Pistache::Async::Promise<ssize_t>
send_pcap_response_header(Pistache::Http::ResponseWriter& response,
                          Pistache::Http::Version version,
                          transfer_context& context);

Pistache::Async::Promise<ssize_t> serve_capture_pcap(transfer_context& context);

} // namespace openperf::packet::capture::pcap

#endif // _OP_PACKET_CAPTURE_PCAP_TRANSFER_HPP_
