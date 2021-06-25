#ifndef _OP_TIMESYNC_SOURCE_SYSTEM_HPP_
#define _OP_TIMESYNC_SOURCE_SYSTEM_HPP_

#include "tl/expected.hpp"
#include "timesync/api.hpp"
#include "timesync/counter.hpp"

namespace openperf {

namespace core {
class event_loop;
}

namespace timesync {

class clock;

namespace source {

struct system
{
    std::reference_wrapper<core::event_loop> loop;
    clock* clock = nullptr;
    unsigned poll_loop_id = 0;
    unsigned poll_count = 0;

    system(core::event_loop& loop_, class clock* clock_);
    ~system();

    system(system& other) = delete;
    system& operator=(system& other) = delete;
};

api::time_source_system to_time_source(const system& source);

} // namespace source
} // namespace timesync
} // namespace openperf

#endif /* _OP_TIMESYNC_SOURCE_SYSTEM_HPP_ */
