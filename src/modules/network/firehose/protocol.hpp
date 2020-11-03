#ifndef _OP_NETWORK_FIREHOSE_PROTOCOL_HPP_
#define _OP_NETWORK_FIREHOSE_PROTOCOL_HPP_

#include "tl/expected.hpp"
#include "string.h"

namespace openperf::network::internal::firehose {

const std::string protocol = "ICP Firehose 2.0";
const size_t protocol_len = 16;

enum action_t { NONE = 0, GET, PUT };

struct request_t
{
    action_t action;
    uint32_t length;
};

struct net_request_t
{
    char protocol[16];
    int32_t action;
    uint32_t length;
} __attribute__((packed));
const size_t net_request_size = sizeof(net_request_t);

inline tl::expected<request_t, int> parse_request(uint8_t*& data,
                                                  size_t& length)
{
    if (length < net_request_size) { return tl::make_unexpected(-1); }
    auto net_request = (net_request_t*)data;

    /* Verify protocol string header */
    if (strncmp(net_request->protocol, protocol.c_str(), protocol_len) != 0) {
        return tl::make_unexpected(-1);
    }

    auto request =
        request_t{.action = static_cast<action_t>(ntohl(net_request->action)),
                  .length = ntohl(net_request->length)};

    /* update pointers to data buffer */
    data += net_request_size;
    length -= net_request_size;

    return request;
}

inline tl::expected<void, int> build_request(const request_t& request,
                                             std::vector<uint8_t>& data)
{
    net_request_t net_request;

    strncpy(net_request.protocol, protocol.c_str(), protocol_len);

    net_request.action = htonl(request.action);
    net_request.length = htonl(request.length);

    data.resize(sizeof(net_request));
    memcpy(data.data(), &net_request, sizeof(net_request));

    return {};
}

} // namespace openperf::network::internal::firehose

#endif