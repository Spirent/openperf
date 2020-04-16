#ifndef _OP_PACKET_CAPTURE_PCAP_HPP_
#define _OP_PACKET_CAPTURE_PCAP_HPP_

#include <pistache/http.h>

namespace openperf::packet::capture {
class reader;
}

namespace openperf::packet::capture::api {

Pistache::Async::Promise<ssize_t>
server_capture_pcap(Pistache::Http::ResponseWriter& response,
                    std::shared_ptr<reader> reader);

}

#endif //_OP_PACKET_CAPTURE_PCAP_HPP_