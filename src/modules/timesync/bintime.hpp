#ifndef _OP_TIMESYNC_BINTIME_HPP_
#define _OP_TIMESYNC_BINTIME_HPP_

#include <chrono>
#include <cmath>
#include <cstdint>

#include <sys/time.h>

namespace openperf::timesync {

struct bintime
{
    time_t bt_sec;    /**< Number of seconds */
    uint64_t bt_frac; /**< Numerator for fractional second;
                       * denominator is 2^64 */

    static constexpr bintime zero() { return {0, 0}; }
};

inline int compare(const bintime& lhs, const bintime& rhs)
{
    return (lhs.bt_sec < rhs.bt_sec
                ? -1
                : lhs.bt_sec > rhs.bt_sec
                      ? 1
                      : lhs.bt_frac < rhs.bt_frac
                            ? -1
                            : lhs.bt_frac > rhs.bt_frac ? 1 : 0);
}

inline bool operator==(const bintime& lhs, const bintime& rhs)
{
    return (compare(lhs, rhs) == 0);
}

inline bool operator!=(const bintime& lhs, const bintime& rhs)
{
    return (compare(lhs, rhs) != 0);
}

inline bool operator<(const bintime& lhs, const bintime& rhs)
{
    return compare(lhs, rhs) < 0;
}

inline bool operator>(const bintime& lhs, const bintime& rhs)
{
    return compare(lhs, rhs) > 0;
}

inline bool operator<=(const bintime& lhs, const bintime& rhs)
{
    return compare(lhs, rhs) <= 0;
}

inline bool operator>=(const bintime& lhs, const bintime& rhs)
{
    return compare(lhs, rhs) >= 0;
}

/***
 * Time conversion routines
 ***/

using namespace std::chrono_literals;
static constexpr auto ns_per_sec = std::chrono::nanoseconds(1s).count();
static constexpr auto us_per_sec = std::chrono::microseconds(1s).count();

inline timespec to_timespec(const struct bintime& src)
{
    using nsec_t = decltype(timespec().tv_nsec);

    return (timespec{.tv_sec = src.bt_sec,
                     .tv_nsec =
                         static_cast<nsec_t>(ns_per_sec * (src.bt_frac >> 32))
                         >> 32});
}

template <typename Rep, typename Period>
inline timespec to_timespec(std::chrono::duration<Rep, Period> src)
{
    auto src_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(src).count();

    return (src < std::chrono::duration<Rep, Period>::zero()
                ? timespec{.tv_sec = src_ns / ns_per_sec - 1,
                           .tv_nsec = ns_per_sec + src_ns % ns_per_sec}
                : timespec{.tv_sec = src_ns / ns_per_sec,
                           .tv_nsec = src_ns % ns_per_sec});
}

inline timespec to_timespec(double src)
{
    using sec_t = decltype(timespec().tv_sec);
    using nsec_t = decltype(timespec().tv_nsec);

    double sec = 0;
    auto frac = modf(src, &sec);

    return (src < 0
                ? timespec{.tv_sec = static_cast<sec_t>(sec - 1),
                           .tv_nsec = static_cast<nsec_t>(
                               ns_per_sec - fabs(frac) * ns_per_sec)}
                : timespec{.tv_sec = static_cast<sec_t>(sec),
                           .tv_nsec = static_cast<nsec_t>(frac * ns_per_sec)});
}

inline timeval to_timeval(const bintime& src)
{
    using usec_t = decltype(timeval().tv_usec);

    return (timeval{.tv_sec = src.bt_sec,
                    .tv_usec = static_cast<usec_t>(
                        (us_per_sec * (src.bt_frac >> 32)) >> 32)});
}

inline timeval to_timeval(const timespec& src)
{
    using usec_t = decltype(timeval().tv_usec);

    return (timeval{.tv_sec = src.tv_sec,
                    .tv_usec = static_cast<usec_t>(
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::nanoseconds(src.tv_nsec))
                            .count())});
}

template <typename Rep, typename Period>
inline timeval to_timeval(std::chrono::duration<Rep, Period> src)
{
    using sec_t = decltype(timeval().tv_sec);
    using usec_t = decltype(timeval().tv_usec);

    using us = std::chrono::duration<int64_t, std::micro>;
    auto src_us = std::chrono::duration_cast<us>(src).count();

    return (src < std::chrono::duration<Rep, Period>::zero()
                ? timeval{.tv_sec = static_cast<sec_t>(src_us / us_per_sec) - 1,
                          .tv_usec = static_cast<usec_t>(us_per_sec
                                                         + src_us % us_per_sec)}
                : timeval{.tv_sec = static_cast<sec_t>(src_us / us_per_sec),
                          .tv_usec = static_cast<usec_t>(src_us % us_per_sec)});
}

inline timeval to_timeval(double src)
{
    using sec_t = decltype(timeval().tv_sec);
    using usec_t = decltype(timeval().tv_usec);

    double sec = 0;
    auto frac = modf(src, &sec);

    return (src < 0
                ? timeval{.tv_sec = static_cast<sec_t>(sec - 1),
                          .tv_usec = static_cast<usec_t>(
                              us_per_sec - fabs(frac) * us_per_sec)}
                : timeval{.tv_sec = static_cast<sec_t>(sec),
                          .tv_usec = static_cast<usec_t>(frac * us_per_sec)});
}

inline bintime to_bintime(const timespec& src)
{
    /* 18446744073 = 2^64 / ns_per_sec */
    static constexpr auto scalar = 18446744073UL;

    return (bintime{.bt_sec = src.tv_sec, .bt_frac = src.tv_nsec * scalar});
}

inline bintime to_bintime(const timeval& src)
{
    /* 18446744073709 = 2^64 / us_per_sec */
    static constexpr auto scalar = 18446744073709UL;

    return (bintime{.bt_sec = src.tv_sec, .bt_frac = src.tv_usec * scalar});
}

template <typename Rep, typename Period>
inline bintime to_bintime(std::chrono::duration<Rep, Period> src)
{
    return (to_bintime(to_timeval(src)));
}

inline bintime to_bintime(double src) { return (to_bintime(to_timespec(src))); }

inline double to_double(const timespec& src)
{
    return (src.tv_sec + (static_cast<double>(src.tv_nsec) / ns_per_sec));
}

inline double to_double(const timeval& src)
{
    return (src.tv_sec + (static_cast<double>(src.tv_usec) / us_per_sec));
}

inline double to_double(const bintime& src)
{
    return (to_double(to_timespec(src)));
}

/* Intelligently convert bintime to a chrono::duration. */
inline std::chrono::nanoseconds to_duration(const bintime& src)
{
    return (
        std::chrono::nanoseconds{(src.bt_sec * ns_per_sec)
                                 + ((ns_per_sec * (src.bt_frac >> 32)) >> 32)});
}

/***
 * Various bintime manipulations
 ***/

inline bintime operator+(const bintime& lhs, const bintime& rhs)
{
    auto sum = lhs;
    sum.bt_frac += rhs.bt_frac;
    if (lhs.bt_frac > sum.bt_frac) { sum.bt_sec++; /* carry the 1 */ }
    sum.bt_sec += rhs.bt_sec;
    return (sum);
}

inline bintime operator+(const bintime& lhs, double rhs)
{
    return (lhs + to_bintime(rhs));
}

inline bintime operator-(const bintime& lhs, const bintime& rhs)
{
    auto diff = lhs;
    diff.bt_frac -= rhs.bt_frac;
    if (lhs.bt_frac < diff.bt_frac) { diff.bt_sec--; /* borrow a 1 */ }
    diff.bt_sec -= rhs.bt_sec;
    return (diff);
}

inline bintime operator-(const bintime& lhs, double rhs)
{
    return (lhs - to_bintime(rhs));
}

template <typename T> inline bintime operator*(const bintime& lhs, T rhs)
{
    static_assert(std::is_integral<T>::value);

    auto mask = uint64_t{0xffffffff};
    auto tmp1 = (lhs.bt_frac & mask) * rhs;
    auto tmp2 = (lhs.bt_frac >> 32) * rhs + (tmp1 >> 32);

    return (bintime{.bt_sec = (lhs.bt_sec * rhs) + (tmp2 >> 32),
                    .bt_frac = (tmp2 << 32) | (tmp1 & mask)});
}

template <typename T> inline bintime operator<<(const bintime& lhs, T rhs)
{
    static_assert(std::is_integral<T>::value);
    assert(0 <= rhs && rhs <= 64);

    return (bintime{.bt_sec = (lhs.bt_sec << rhs) | (lhs.bt_frac >> (64 - rhs)),
                    .bt_frac = lhs.bt_frac << rhs});
}

template <typename T> inline bintime operator>>(const bintime& lhs, T rhs)
{
    static_assert(std::is_integral<T>::value);
    assert(0 <= rhs && rhs <= 64);

    return (
        bintime{.bt_sec = (lhs.bt_sec >> rhs),
                .bt_frac = (lhs.bt_frac >> rhs) | lhs.bt_sec << (64 - rhs)});
}

} // namespace openperf::timesync

#endif /* _OP_TIMESYNC_BINTIME_HPP_ */
