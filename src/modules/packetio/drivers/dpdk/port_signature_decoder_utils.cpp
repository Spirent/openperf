/*
 *----------------------------------------------------------------------
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------
 */

#include <limits.h>
#include <sys/time.h>

#include "timesync/chrono.hpp"

namespace openperf::packetio::dpdk::port::utils {

namespace impl {

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400 * (31 + 29))

#define DAYS_PER_400Y (365 * 400 + 97)
#define DAYS_PER_100Y (365 * 100 + 24)
#define DAYS_PER_4Y (365 * 4 + 1)

int seconds_to_year(time_t t)
{
    static const char days_in_month[] = {
        31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};

    /* Reject time_t values whose year would overflow int */
    if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL) return -1;

    auto secs = t - LEAPOCH;
    auto days = secs / 86400;
    auto remsecs = secs % 86400;
    if (remsecs < 0) { days--; }

    auto qc_cycles = days / DAYS_PER_400Y;
    auto remdays = days % DAYS_PER_400Y;
    if (remdays < 0) {
        remdays += DAYS_PER_400Y;
        qc_cycles--;
    }

    auto c_cycles = remdays / DAYS_PER_100Y;
    if (c_cycles == 4) c_cycles--;
    remdays -= c_cycles * DAYS_PER_100Y;

    auto q_cycles = remdays / DAYS_PER_4Y;
    if (q_cycles == 25) q_cycles--;
    remdays -= q_cycles * DAYS_PER_4Y;

    auto remyears = remdays / 365;
    if (remyears == 4) remyears--;
    remdays -= remyears * 365;

    auto years = remyears + 4 * q_cycles + 100 * c_cycles + 400 * qc_cycles;

    auto months = 0;
    for (; days_in_month[months] <= remdays; months++)
        remdays -= days_in_month[months];

    if (years + 100 > INT_MAX || years + 100 < INT_MIN) return -1;

    /*
     * Need to account for using February as the last month in this
     * calculation.
     */
    return (static_cast<int>(years + 100 + (months + 2 >= 12 ? 1 : 0)));
}

time_t year_to_seconds(int year)
{
    bool is_leap = false;
    if (year - 2ULL <= 136) {
        int y = year;
        int leaps = (y - 68) >> 2;
        if (!((y - 68) & 3)) { leaps--; }
        return 31536000 * (y - 70) + 86400 * leaps;
    }

    int cycles, centuries, leaps, rem;

    cycles = (year - 100) / 400;
    rem = (year - 100) % 400;
    if (rem < 0) {
        cycles--;
        rem += 400;
    }
    if (!rem) {
        is_leap = true;
        centuries = 0;
        leaps = 0;
    } else {
        if (rem >= 200) {
            if (rem >= 300)
                centuries = 3, rem -= 300;
            else
                centuries = 2, rem -= 200;
        } else {
            if (rem >= 100)
                centuries = 1, rem -= 100;
            else
                centuries = 0;
        }
        if (!rem) {
            is_leap = false;
            leaps = 0;
        } else {
            leaps = rem / 4;
            rem %= 4;
            is_leap = !rem;
        }
    }

    leaps += 97 * cycles + 24 * centuries - (is_leap ? 1 : 0);

    return (year - 100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}

} // namespace impl

/*
 * Both of the utility functions above were intended for use with
 * `tm structs` as used by the standard C library.  Hence, we need
 * to offset the year by 1900.
 */
static constexpr auto impl_offset = 1900;

static int seconds_to_year(time_t seconds)
{
    return (impl::seconds_to_year(seconds) + impl_offset);
}

static time_t year_to_seconds(int year)
{
    return (impl::year_to_seconds(year - impl_offset));
}

time_t get_timestamp_epoch_offset()
{
    using clock = timesync::chrono::realtime;

    return (year_to_seconds(seconds_to_year(clock::to_time_t(clock::now()))));
}

} // namespace openperf::packetio::dpdk::port::utils
