#ifndef _OP_TIMESYNC_HISTORY_HPP_
#define _OP_TIMESYNC_HISTORY_HPP_

#include <functional>
#include <set>

#include "timesync/bintime.hpp"
#include "timesync/counter.hpp"

namespace openperf::timesync {

struct timestamp
{
    uint64_t Ta;
    bintime Tb;
    bintime Te;
    uint64_t Tf;
};

/**
 * A timestamp for internal use.  This is congruent with NTPv4 packet
 * timestamps and is compatible with our internal "bintime" timestamps.
 * We use this version because we don't need the extra resolution of bintime
 */
struct ntp_timestamp
{
    int32_t bt_sec;
    uint32_t bt_frac;
};

inline bool operator==(const ntp_timestamp& lhs, const ntp_timestamp& rhs)
{
    return (lhs.bt_sec == rhs.bt_sec && lhs.bt_frac == rhs.bt_frac);
}

class history
{
    /**
     * Structure containing the four timestamps provided by an NTP client/server
     * exchange as well as the local clock period estimate.  For space
     * efficiency reasons, we delta encode the timestamps.
     */
    struct timesync_data
    {
        counter::hz f_local; /**< Local guestimate of clock frequency */
        uint64_t Ta;         /**< Client's transmit time of request, in ticks */
        ntp_timestamp Tb;    /**< Server's receive time of client request */
        uint32_t offset_Te;  /**< Delta between server's receive of request and
                              * transmit of reply, as the numerator of a 32 bit
                              * fraction */
        uint32_t offset_Tf; /**< Delta between client's transmit time of request
                             * and receive time of reply, in ticks. */
    } __attribute((packed));

    /* Compare data using the timestamp of the remote server */
    struct timesync_data_compare
    {
        bool operator()(const timesync_data& lhs,
                        const timesync_data& rhs) const
        {
            if (lhs.Tb.bt_sec < rhs.Tb.bt_sec
                || (lhs.Tb.bt_sec == rhs.Tb.bt_sec
                    && lhs.Tb.bt_frac < rhs.Tb.bt_frac)) {
                return (true);
            }
            return (false);
        }
    };

    static timestamp to_timestamp(const timesync_data& data);

public:
    bool empty() const noexcept;
    size_t size() const noexcept;

    void clear() noexcept;
    bool contains(const timestamp& ts) const noexcept;
    void insert(const timestamp& ts, counter::hz f_local);

    bintime duration() const noexcept;

    using history_container = std::set<timesync_data, timesync_data_compare>;
    history_container::iterator lower_bound(time_t time) const;
    history_container::iterator upper_bound(time_t time) const;

    void prune(time_t time);

    // template <typename Argument>
    // using history_apply_function = void (*)(const timestamp& ts,
    //                                        counter::hz f_local,
    //                                        Argument& arg);

    using history_range =
        std::pair<history_container::iterator, history_container::iterator>;
    template <typename Function, typename Argument>
    void apply(const history_range& range, Function&& f, Argument& arg)
    {
        std::for_each(range.first, range.second, [&](const auto& item) {
            std::invoke(std::forward<Function>(f),
                        to_timestamp(item),
                        item.f_local,
                        arg);
        });
    }

private:
    history_container m_history;
};

} // namespace openperf::timesync
#endif /* _OP_TIMESYNC_HISTORY_HPP */
