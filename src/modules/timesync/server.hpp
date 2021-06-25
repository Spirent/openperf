#ifndef _OP_TIMESYNC_SERVER_HPP_
#define _OP_TIMESYNC_SERVER_HPP_

#include <memory>
#include <variant>

#include "core/op_core.h"
#include "timesync/api.hpp"
#include "timesync/clock.hpp"
#include "timesync/source_ntp.hpp"
#include "timesync/source_system.hpp"

namespace openperf::timesync::api {

class server
{
public:
    /*
     * Since time sources hook into the event loop, they are
     * neither movable nor copyable. Hence, we have to store
     * them as pointers.
     */
    using time_source = std::variant<std::unique_ptr<source::system>,
                                     std::unique_ptr<source::ntp>>;

    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request_time_counters&);
    reply_msg handle_request(const request_time_keeper&);
    reply_msg handle_request(const request_time_sources&);
    reply_msg handle_request(const request_time_source_add&);
    reply_msg handle_request(const request_time_source_del&);

private:
    core::event_loop& m_loop;
    std::unique_ptr<clock> m_clock;
    std::unique_ptr<void, op_socket_deleter> m_socket;

    std::map<std::string, time_source> m_sources;

    std::vector<std::unique_ptr<counter::timecounter>> m_timecounters;
};

} // namespace openperf::timesync::api

#endif /* _OP_TIMESYNC_SERVER_HPP_ */
