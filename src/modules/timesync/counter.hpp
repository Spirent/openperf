#ifndef _OP_TIMESYNC_COUNTER_HPP_
#define _OP_TIMESYNC_COUNTER_HPP_

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>

#include "core/op_init_factory.hpp"
#include "core/op_uuid.hpp"
#include "timesync/bintime.hpp"
#include "units/rate.hpp"

namespace openperf::timesync {

namespace counter {

using hz = units::rate<uint64_t>;
using ticks = uint64_t;

struct timecounter : openperf::core::init_factory<timecounter>
{
    const core::uuid id = core::uuid::random();

    timecounter(Key){};
    virtual ~timecounter() = default;

    virtual std::string_view name() const = 0; /**< time counter name */
    virtual ticks now() const = 0;             /**< read the counter value */
    virtual hz frequency() const = 0; /**< best guess of counter frequency */
    virtual int static_priority() const = 0; /**< relative counter quality,
                                              * lower is better */
};

extern std::atomic<timecounter*> timecounter_now;

inline hz frequency()
{
    auto tc = timecounter_now.load(std::memory_order_relaxed);
    return (tc->frequency());
}

inline ticks now()
{
    auto tc = timecounter_now.load(std::memory_order_relaxed);
    return (tc->now());
}

} // namespace counter

inline bintime to_bintime(counter::ticks ticks, counter::hz freq)
{
    using sec_t = decltype(bintime().bt_sec);
    return (bintime{.bt_sec = static_cast<sec_t>(ticks / freq.count()),
                    .bt_frac = (ticks % freq.count())
                               * (((1ULL << 63) / freq.count()) << 1)});
}

} // namespace openperf::timesync

#endif /* _OP_TIMESYNC_COUNTER_HPP_ */
