#ifndef _OP_TIMESYNC_SOURCE_NTP_HPP_
#define _OP_TIMESYNC_SOURCE_NTP_HPP_

#include <chrono>
#include <memory>
#include <optional>

#include <sys/socket.h>
#include <netdb.h>

#include "tl/expected.hpp"
#include "timesync/api.hpp"
#include "timesync/counter.hpp"
#include "timesync/socket.hpp"

namespace openperf {

namespace core {
class event_loop;
}

namespace timesync {

class clock;

namespace source {

struct ntp
{
    struct addrinfo_deleter
    {
        void operator()(addrinfo* ai) { freeaddrinfo(ai); }
    };

    using addrinfo_ptr = std::unique_ptr<addrinfo, addrinfo_deleter>;

    std::reference_wrapper<core::event_loop> loop;
    addrinfo_ptr addrinfo;
    timesync::ntp::socket socket;
    clock* clock;
    unsigned poll_loop_id = 0;
    struct
    {
        std::chrono::seconds poll_period = 64s;
        unsigned rx = 0;
        unsigned tx = 0;
        std::optional<uint8_t> stratum = std::nullopt; /* unsynchronized */
    } stats;
    counter::ticks last_tx = 0;
    bintime expected_origin = bintime::zero();

    ntp(core::event_loop& loop, class clock* clock, struct addrinfo* ai);
    ~ntp();

    ntp(ntp&) = delete;
    ntp& operator=(ntp&) = delete;
};

api::time_source_ntp to_time_source(const ntp& source);

} // namespace source
} // namespace timesync
} // namespace openperf

#endif /* _OP_TIMESYNC_SOURCE_NTP_HPP_ */
