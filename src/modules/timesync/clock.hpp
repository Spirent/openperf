#ifndef _OP_TIMESYNC_CLOCK_HPP_
#define _OP_TIMESYNC_CLOCK_HPP_

#include <functional>
#include <set>

#include "tl/expected.hpp"

#include "digestible/digestible.h"
#include "timesync/bintime.hpp"
#include "timesync/counter.hpp"
#include "timesync/history.hpp"

namespace openperf::timesync {

class clock
{
public:
    using update_function = std::function<int(
        const bintime& time, counter::ticks ticks, counter::hz freq)>;
    clock(update_function update);

    void reset();
    bool synced() const;
    tl::expected<void, int> update(counter::ticks Ta,
                                   const bintime& Tb,
                                   const bintime& Te,
                                   counter::ticks Tf);

    /* Frequency calculations always involve a result and an error */
    using freq_result = std::pair<counter::hz, counter::hz>;
    using theta_result = std::pair<bintime, double>;
    using rtt_container = digestible::tdigest<counter::ticks, unsigned>;

private:
    std::optional<freq_result> do_rate_estimation(const timestamp& ts);
    std::optional<freq_result> do_local_rate_estimation(const timestamp& ts);
    counter::ticks do_level_shift_detection(const timestamp& ts);
    void do_offset_sync(const timestamp& ts);

    update_function m_update;
    history m_history;

    struct data
    {
        struct
        {
            freq_result current = {counter::hz{0}, counter::hz{0}};
            counter::ticks last_update = 0;
        } f_hat;
        struct
        {
            freq_result current = {counter::hz{0}, counter::hz{0}};
            counter::ticks last_update = 0;
        } f_local;
        struct
        {
            theta_result current = {bintime::zero(), 0};
            counter::ticks last_update = 0;
        } theta_hat;
        bintime K = bintime::zero();
    };
    data m_data;

    struct stats
    {
        rtt_container rtts = rtt_container(32);
        size_t timestamps = 0;
        size_t f_hat = 0;
        size_t f_local = 0;
        size_t theta_hat = 0;
        size_t updates = 0;
        counter::ticks last_update = 0;
    };
    stats m_stats;
};

} // namespace openperf::timesync

#endif /* _OP_TIMESYNC_CLOCK_HPP_ */
