#include <algorithm>
#include <iterator>

#include <netinet/in.h>

#include "packet/analyzer/statistics/flow/header_utils.hpp"

namespace openperf::packet::analyzer::statistics::flow {

const uint8_t* skip_ipv6_extension_headers(const uint8_t* cursor,
                                           const uint8_t* end,
                                           uint8_t next_header)
{
    struct ipv6_extension
    {
        uint8_t next_header;
        uint8_t length;
    };

    while (cursor + sizeof(ipv6_extension) < end) {
        switch (next_header) {
        case IPPROTO_HOPOPTS:
        case IPPROTO_ROUTING:
        case IPPROTO_DSTOPTS: {
            auto* ext = reinterpret_cast<const ipv6_extension*>(cursor);
            cursor += std::min(static_cast<ptrdiff_t>(ext->length),
                               std::distance(cursor, end));
            next_header = ext->next_header;
            break;
        }
        case IPPROTO_FRAGMENT:
        case IPPROTO_NONE: {
            /* Either value indicates we're done processing headers */
            auto* ext = reinterpret_cast<const ipv6_extension*>(cursor);
            cursor += std::min(static_cast<ptrdiff_t>(ext->length),
                               std::distance(cursor, end));
            return (cursor);
        }
        default:
            /* Bail! */
            return (cursor);
        }
    }

    return (end);
}

} // namespace openperf::packet::analyzer::statistics::flow
