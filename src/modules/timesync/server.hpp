#ifndef _OP_TIMESYNC_SERVER_HPP_
#define _OP_TIMESYNC_SERVER_HPP_

//#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "core/op_core.h"
#include "timesync/api.hpp"
#include "timesync/clock.hpp"
#include "timesync/counter.hpp"
#include "timesync/socket.hpp"

namespace openperf::timesync::api {

struct ntp_server_state
{
    struct addrinfo_deleter
    {
        void operator()(addrinfo* ai) { freeaddrinfo(ai); }
    };

    using addrinfo_ptr = std::unique_ptr<addrinfo, addrinfo_deleter>;
    addrinfo_ptr addrinfo;
    ntp::socket socket;
    clock* clock;
    unsigned poll_loop_id = 0;
    struct
    {
        unsigned rx = 0;
        unsigned tx = 0;
        std::optional<uint8_t> stratum = std::nullopt; /* unsynchronized */
    } stats;
    counter::ticks last_tx = 0;

    ntp_server_state(struct addrinfo* ai_, class clock* clock_)
        : addrinfo(ai_)
        , socket(ai_)
        , clock(clock_)
    {}
};

class server
{
    core::event_loop& m_loop;
    std::unique_ptr<clock> m_clock;
    std::unique_ptr<void, op_socket_deleter> m_socket;

    using ntp_server_ptr = std::unique_ptr<ntp_server_state>;
    std::unordered_map<std::string, ntp_server_ptr> m_sources;

    std::vector<std::unique_ptr<counter::timecounter>> m_timecounters;

public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request_time_counters&);
    reply_msg handle_request(const request_time_keeper&);
    reply_msg handle_request(const request_time_sources&);
    reply_msg handle_request(const request_time_source_add&);
    reply_msg handle_request(const request_time_source_del&);
};

} // namespace openperf::timesync::api

#endif /* _OP_TIMESYNC_SERVER_HPP_ */
