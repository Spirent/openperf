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
    using rtt_container = digestible::tdigest<counter::ticks, unsigned>;

    std::optional<counter::hz> frequency() const;
    std::optional<counter::hz> frequency_error() const;
    std::optional<counter::hz> local_frequency() const;
    std::optional<counter::hz> local_frequency_error() const;
    bintime offset() const;
    std::optional<bintime> theta() const;

    size_t nb_frequency_accept() const;
    size_t nb_frequency_reject() const;
    size_t nb_local_frequency_accept() const;
    size_t nb_local_frequency_reject() const;
    size_t nb_theta_accept() const;
    size_t nb_theta_reject() const;
    size_t nb_timestamps() const;

    std::optional<double> rtt_maximum() const;
    std::optional<double> rtt_median() const;
    std::optional<double> rtt_minimum() const;
    size_t rtt_size() const;

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
            bintime current = bintime::zero();
            counter::ticks last_update = 0;
        } theta_hat;
        bintime K = bintime::zero();
    };
    data m_data;

    struct stats
    {
        rtt_container rtts = rtt_container(16);
        size_t f_hat_accept = 0;
        size_t f_hat_reject = 0;
        size_t f_local_accept = 0;
        size_t f_local_reject = 0;
        size_t theta_hat_accept = 0;
        size_t theta_hat_reject = 0;
        counter::ticks last_update = 0;
    };
    stats m_stats;
};

} // namespace openperf::timesync

#endif /* _OP_TIMESYNC_CLOCK_HPP_ */
