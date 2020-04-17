#ifndef _OP_PACKET_CAPTURE_PCAP_HPP_
#define _OP_PACKET_CAPTURE_PCAP_HPP_

#include <pistache/http.h>

namespace openperf::packet::capture {
class transfer_context;
}

namespace openperf::packet::capture::api {

std::shared_ptr<transfer_context>
create_pcap_transfer_context(Pistache::Http::ResponseWriter& response);

Pistache::Async::Promise<ssize_t>
send_pcap_response_header(Pistache::Http::ResponseWriter& response,
                          std::shared_ptr<transfer_context> context);

Pistache::Async::Promise<ssize_t>
serve_capture_pcap(std::shared_ptr<transfer_context> context);

} // namespace openperf::packet::capture::api

#endif //_OP_PACKET_CAPTURE_PCAP_HPP_