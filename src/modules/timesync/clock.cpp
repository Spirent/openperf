#include <chrono>
#include <cinttypes>

#include "core/op_log.h"
#include "timesync/clock.hpp"

namespace openperf::timesync {

/* Clock algorithm and system constants */

using namespace std::chrono_literals;
/* timescale used for offset calculation */
static constexpr auto tau_star = 1200s;
/* timescale used for frequency calculation */
static constexpr auto tau_local = 3600s;
/* acceptable variance in min/max window size of local frequency calculation */
static constexpr auto tau_local_window_max = 300s;
/* drop timestamps over this age */
static constexpr auto max_history = 2 * tau_local;

/* the amount of noise in timestamp measurements */
using ppm = std::ratio<1, 1000000>;
static constexpr auto noise =
    std::ratio_multiply<std::ratio<15>, ppm>(); /* 15 ppm */

/* maximum deviation from f_hat when updating f_local, in ppm */
static constexpr auto f_local_ppm_limit =
    std::ratio_multiply<std::ratio<5, 100>, ppm>(); /* 0.05 ppm */
static constexpr auto f_hat_ppm_limit =
    std::ratio_multiply<std::ratio<3, 100>, ppm>(); /* 0.03 ppm */
static constexpr auto theta_ppm_limit =
    std::ratio_multiply<std::ratio<1, 100>, ppm>(); /* 0.01 ppm */

template <typename T, typename U> T get_value(const std::pair<T, U>& pair)
{
    return (std::get<0>(pair));
}

template <typename T, typename U> U get_error(const std::pair<T, U>& pair)
{
    return (std::get<1>(pair));
}

counter::ticks to_rtt(const timestamp& ts) { return (ts.Tf - ts.Ta); }

time_t to_time(const timestamp& ts) { return (ts.Tb.bt_sec); }

/*
 * As the clock runs and collects more timestamps, we can generate better
 * estimates. A consequence of this is that we expect clock parameter
 * adjustments to decrease in scale as time passes. We use the following
 * function to calculate a ppm value based on the number of updates we have
 * calculated.
 */
template <std::intmax_t Num, std::intmax_t Denom>
static double get_threshold_ppm(size_t count,
                                const std::ratio<Num, Denom>& min_ratio)
{
    auto min = static_cast<double>(min_ratio.num) / min_ratio.den;
    static constexpr int threshold_offset = 4;
    return (count <= threshold_offset
                ? 10
                : std::max(min,
                           (10 / (1 + std::pow(count - threshold_offset, 2)))));
}

template <std::intmax_t Num, std::intmax_t Denom>
static bool offset_delta_exceeded(const bintime& theta_old,
                                  const bintime& theta_new,
                                  const bintime& delta_t,
                                  size_t count,
                                  const std::ratio<Num, Denom>& threshold)
{
    if (theta_old == bintime::zero()) return (false);

    auto delta = to_double(theta_new - theta_old);
    auto ppm_adjust = delta / to_double(delta_t) / 1e6;
    return (ppm_adjust > get_threshold_ppm(count, threshold));
}

template <std::intmax_t Num, std::intmax_t Denom>
static bool rate_delta_exceeded(counter::hz older,
                                counter::hz newer,
                                const bintime& delta_t,
                                size_t count,
                                const std::ratio<Num, Denom>& threshold)
{
    if (!older.count() || delta_t == bintime::zero()) {
        return (false); /* can't make any comparisons */
    }

    auto delta_hz = newer > older ? newer - older : older - newer;
    auto ppm_adjust = delta_hz
                      / std::chrono::duration_cast<std::chrono::microseconds>(
                            to_duration(delta_t))
                            .count();
    return (ppm_adjust.count() > get_threshold_ppm(count, threshold));
}

struct rate_sync_timestamps
{
    counter::ticks threshold;
    std::optional<timestamp> i;
    std::optional<timestamp> j;
};

static void find_rate_sync_timestamps(const timestamp& ts,
                                      counter::hz /* unused */,
                                      rate_sync_timestamps& stamps)
{
    if (to_rtt(ts) <= stamps.threshold) {
        /* Possible datum discovered; do we have a j? */
        if (!stamps.j) {
            stamps.j = ts;
        } else if (!stamps.i || stamps.i->Tf < ts.Tf) {
            stamps.i = ts;
        }
    }
}

static void find_lowest_error_timestamp(const timestamp& ts,
                                        counter::hz /* unused */,
                                        std::optional<timestamp>& k)
{
    /* Update k if it's not set or we find a lower RTT value */
    if (!k || (to_rtt(ts) < to_rtt(*k))) { k = ts; }
}

static bintime calculate_uncorrected_time(const bintime& K,
                                          counter::ticks ticks,
                                          counter::hz freq)
{
    return (to_bintime(ticks, freq) + K);
}

static bintime calculate_absolute_time(const bintime& theta,
                                       const bintime& K,
                                       counter::ticks ticks,
                                       counter::hz freq)
{
    return (calculate_uncorrected_time(K, ticks, freq) - theta);
}

static bintime
calculate_theta(const timestamp& ts, const bintime& K, counter::hz freq)
{
    auto Cu_ta = calculate_uncorrected_time(K, ts.Ta, freq);
    auto Cu_tf = calculate_uncorrected_time(K, ts.Tf, freq);
    auto local = (Cu_ta + Cu_tf) / 2;  /* average */
    auto remote = (ts.Tb + ts.Te) / 2; /* average */
    return (local - remote);
}

struct theta_hat_calc_token
{
    counter::ticks min_rtt; /**< minimum rtt for calculation */
    counter::hz f_hat;      /**< current tick frequency estimate */
    bintime K;              /**< uncorrected clock offset */
    bintime t;              /**< reference point for time deltas */
    struct
    {
        double num; /* weighted sum of theta estimates in seconds */
        double den; /* weighted sum */
    } theta;
};

static void accumulate_theta_hat(const timestamp& ts,
                                 counter::hz f_local,
                                 theta_hat_calc_token& token)
{
    auto min_rtt = std::min(to_rtt(ts), token.min_rtt);
    auto e = 4 * token.f_hat.count() * noise.num / noise.den;
    auto e_i = to_rtt(ts) - min_rtt;
    auto t_i = to_bintime(ts.Tf, token.f_hat);
    auto delta_t = token.t - t_i;
    auto e_i_t = e_i + (1e-7 * to_double(delta_t));
    auto omega_i = exp(-1 * (pow(e_i_t / e, 2)));

    if (omega_i > std::numeric_limits<decltype(omega_i)>::epsilon()) {
        auto theta_i = calculate_theta(ts, token.K, token.f_hat);

        using double_hz = units::rate<double>;
        auto gamma_hat = f_local != f_local.zero()
                             ? 1
                                   - (units::rate_cast<double_hz>(token.f_hat)
                                      / units::rate_cast<double_hz>(f_local))
                             : 0;
        token.theta.num +=
            omega_i * (to_double(theta_i) + gamma_hat * to_double(delta_t));
        token.theta.den += omega_i;
    }
}

static uint64_t calculate_tick_error(counter::ticks error, counter::ticks delta)
{
    return (error * 1000000000 / delta);
}

static counter::hz to_hz(double d)
{
    assert(d >= 0);
    return (counter::hz(static_cast<unsigned>(std::trunc(d))));
}

/*
 * Ideally, we would like to read the tick clock and the system clock
 * at the same instance, but of course, that's not possible.  So instead
 * we guestimate the tick clock by interpolating between two reads.
 * Additionally, we set a 1 us limit on the delta between the reads to
 * avoid scheduling delays.
 */
static bintime get_host_offset()
{
    using clock = std::chrono::high_resolution_clock;
    auto B_limit = counter::frequency().count() / 1000000;
    auto B_step = B_limit / 100;

    auto B = counter::ticks{0};
    auto offset = bintime::zero();

    do {
        auto ticks = counter::now();
        auto clock_now = clock::now();
        B = counter::now() - ticks;
        auto ticks_now = ticks + B / 2;
        B_limit += B_step;
        offset = to_bintime(clock_now.time_since_epoch())
                 - to_bintime(ticks_now, counter::frequency());
    } while (B > B_limit);

    return (offset);
}

clock::clock(clock::update_function update)
    : m_update(std::move(update))
{
    m_data.K = get_host_offset();
}

void clock::reset()
{
    m_history.clear();

    m_data = data{.K = get_host_offset()};

    m_stats = stats{};
    assert(m_stats.rtts.centroid_count() == 0);
}

bool clock::synced() const
{
    bool is_synced = false;

    if (m_stats.theta_hat_accept > 1
        && get_value(m_data.f_hat.current).count()) {
        auto delta_ticks = counter::now() - m_data.f_hat.last_update;
        auto delta = to_bintime(delta_ticks, get_value(m_data.f_hat.current));
        assert(delta.bt_sec >= 0); /* negative time is bad, yo! */
        is_synced = std::chrono::seconds(delta.bt_sec) <= 2 * tau_star;
    }

    return (is_synced);
}

static clock::freq_result calculate_tick_freq(const timestamp& i,
                                              const timestamp& j,
                                              counter::ticks min_rtt)
{
    using seconds = std::chrono::duration<double>;
    auto freq_up =
        to_hz(static_cast<double>(i.Ta - j.Ta)
              / std::chrono::duration_cast<seconds>(to_duration(i.Tb - j.Tb))
                    .count());
    auto freq_down =
        to_hz(static_cast<double>(i.Tf - j.Tf)
              / std::chrono::duration_cast<seconds>(to_duration(i.Te - j.Te))
                    .count());

    assert(min_rtt <= to_rtt(i));
    assert(min_rtt <= to_rtt(j));
    auto e_i = to_rtt(i) - min_rtt;
    auto e_j = to_rtt(j) - min_rtt;
    auto e_up = calculate_tick_error(e_i + e_j, i.Ta - j.Ta);
    auto e_down = calculate_tick_error(e_i + e_j, i.Tf - j.Tf);

    return (clock::freq_result((freq_up + freq_down) / 2, (e_up + e_down) / 2));
}

std::optional<clock::freq_result> clock::do_rate_estimation(const timestamp& ts)
{
    /*
     * Perform local rate estimation using only the best 50% of timestamps.
     * If the latest timestamp is within our RTT threshold, it becomes our
     * preferred i stamp.
     */
    auto stamps = rate_sync_timestamps{
        .threshold = static_cast<counter::ticks>(m_stats.rtts.quantile(50.0)),
        .i = std::nullopt,
        .j = std::nullopt};

    if (to_rtt(ts) < stamps.threshold) { stamps.i = ts; }

    auto range =
        std::make_pair(m_history.lower_bound(to_time(ts) - max_history.count()),
                       m_history.upper_bound(to_time(ts)));
    m_history.apply(range, find_rate_sync_timestamps, stamps);

    if (stamps.i && stamps.j) {
        return (calculate_tick_freq(*stamps.i, *stamps.j, m_stats.rtts.min()));
    }

    return (std::nullopt);
}

std::optional<clock::freq_result>
clock::do_local_rate_estimation(const timestamp& ts)
{
    auto runtime = std::chrono::seconds(m_history.duration().bt_sec);
    if (runtime < tau_local) {
        /* We haven't been running long enough for an estimate */
        return (std::nullopt);
    }

    auto far = std::min(tau_local, runtime);
    auto range = std::min(tau_local_window_max, far / 2);

    std::optional<timestamp> i = ts; /* include current stamp in this search */
    auto range_i =
        std::make_pair(m_history.lower_bound(to_time(ts) - range.count()),
                       m_history.upper_bound(to_time(ts)));
    m_history.apply(range_i, find_lowest_error_timestamp, i);

    auto j = std::optional<timestamp>();
    auto range_j = std::make_pair(
        m_history.lower_bound(to_time(ts) - far.count() - range.count()),
        m_history.upper_bound(to_time(ts) - far.count() + range.count()));
    m_history.apply(range_j, find_lowest_error_timestamp, j);

    if (to_rtt(*i) && to_rtt(*j)) {
        return (calculate_tick_freq(*i, *j, m_stats.rtts.min()));
    }

    return (std::nullopt);
}

static void filter_rtt_values(clock::rtt_container& rtts, counter::ticks min)
{
    auto items = rtts.get();
    rtts.reset();
    std::for_each(std::begin(items), std::end(items), [&](const auto& item) {
        if (min < item.first) { rtts.insert(item.first, item.second); }
    });

    /* Add the minimum to the collection, too! */
    rtts.insert(min);

    /* XXX: Force a digest update, otherwise stats can be wrong? */
    rtts.merge();
}

counter::ticks clock::do_level_shift_detection(const timestamp& ts)
{
    auto r_hat_stamp = std::optional<timestamp>();
    auto range = std::make_pair(
        m_history.lower_bound(to_time(ts) - (tau_local / 2).count()),
        m_history.upper_bound(to_time(ts)));
    m_history.apply(range, find_lowest_error_timestamp, r_hat_stamp);

    auto r_hat_s = to_rtt(*r_hat_stamp);
    auto r_hat = m_stats.rtts.min();
    auto threshold = (get_value(m_data.f_hat.current).count()) * 16 * noise.num
                     / noise.den; /* 4E */

    if (r_hat < r_hat_s && threshold < (r_hat - r_hat_s)) {
        OP_LOG(OP_LOG_WARNING,
               "Timesync level shift detected. "
               "Adjusting minimum RTT (%.06f --> %.06f)\n",
               to_double(to_bintime(r_hat, get_value(m_data.f_hat.current))),
               to_double(to_bintime(r_hat_s, get_value(m_data.f_hat.current))));

        /*
         * Filter out the low RTT values to that we can continue to make
         * clock sync estimates.
         */
        filter_rtt_values(m_stats.rtts, r_hat_s);
        m_history.prune_rtts(r_hat_s);
        return (r_hat_s);
    }

    return (r_hat);
}

void clock::do_offset_sync(const timestamp& ts)
{
    auto min_rtt = do_level_shift_detection(ts);

    auto token = theta_hat_calc_token{
        .min_rtt = min_rtt,
        .f_hat = get_value(m_data.f_hat.current),
        .K = m_data.K,
        .t = to_bintime(ts.Tf, get_value(m_data.f_hat.current))};

    auto range =
        std::make_pair(m_history.lower_bound(to_time(ts) - tau_star.count()),
                       m_history.upper_bound(to_time(ts)));
    m_history.apply(range, accumulate_theta_hat, token);

    if (token.theta.den == 0) { return; } /* no suitable estimates */

    auto theta = to_bintime(token.theta.num / token.theta.den);

    /*
     * After the first update, the clock should not move very much between
     * successive updates. Verify that we're not about to screw up the clock.
     */
    auto delta_t =
        (m_stats.f_hat_accept ? to_bintime(ts.Tf - m_stats.last_update,
                                           get_value(m_data.f_hat.current))
                              : bintime::zero());

    if (!offset_delta_exceeded(m_data.theta_hat.current,
                               theta,
                               delta_t,
                               m_stats.theta_hat_accept,
                               theta_ppm_limit)) {
        m_data.theta_hat.current = theta;
        m_data.theta_hat.last_update = ts.Tf;
        m_stats.theta_hat_accept++;

        auto now = calculate_absolute_time(
            theta, m_data.K, ts.Tf, get_value(m_data.f_hat.current));

        auto freq = (get_value(m_data.f_local.current) != counter::hz::zero()
                         ? get_value(m_data.f_local.current)
                         : get_value(m_data.f_hat.current));

        m_update(now, ts.Tf, freq);
        m_stats.last_update = ts.Tf;
    } else {
        OP_LOG(OP_LOG_WARNING,
               "theta_hat update rejected "
               "(%.06f --> %.06f, ppm = %.06f)\n",
               to_double(m_data.theta_hat.current),
               to_double(theta),
               get_threshold_ppm(m_stats.theta_hat_accept, theta_ppm_limit));
        m_stats.theta_hat_reject++;
    }
}

tl::expected<void, int>
clock::update(uint64_t Ta, const bintime& Tb, const bintime& Te, uint64_t Tf)
{
    auto stamp = timestamp{.Ta = Ta, .Tb = Tb, .Te = Te, .Tf = Tf};

    /* Filter out duplicates; they cause problems */
    if (m_history.contains(stamp)) {
        OP_LOG(OP_LOG_WARNING, "duplicate timestamp received\n");
        return (tl::make_unexpected(EEXIST));
    }

    /* Update RTT stats */
    m_stats.rtts.insert(to_rtt(stamp));
    m_stats.rtts.merge();

    /*
     * Update our f_hat estimate. Even though we use our best clock
     * transactions to generate the estimate, we still perform basic
     * sanity checking on our calculation.
     */
    auto f_hat = do_rate_estimation(stamp);
    auto delta_t =
        (m_stats.f_hat_accept ? to_bintime(Tf - m_data.f_hat.last_update,
                                           get_value(m_data.f_hat.current))
                              : bintime::zero());

    if (f_hat
        && !rate_delta_exceeded(get_value(m_data.f_hat.current),
                                get_value(*f_hat),
                                delta_t,
                                m_stats.f_hat_accept,
                                f_hat_ppm_limit)) {
        m_data.f_hat.current = *f_hat;
        m_data.f_hat.last_update = Tf;
        m_stats.f_hat_accept++;
    } else if (f_hat) {
        OP_LOG(OP_LOG_WARNING,
               "f_hat update rejected "
               "(%" PRIu64 " --> %" PRIu64 ", ppm = %.06f)\n",
               get_value(m_data.f_hat.current).count(),
               get_value(*f_hat).count(),
               get_threshold_ppm(m_stats.f_hat_accept, f_hat_ppm_limit));
        m_stats.f_hat_reject++;
    }

    /*
     * Try to calculate f_local. This calculation requires a clock to run
     * for at least tau_local seconds, so we might not have one. That's ok.
     */
    auto f_local = do_local_rate_estimation(stamp);

    /*
     * We want to accept the most recent, reasonable looking value for
     * f_local. However, since f_local is determined by relative instead
     * of absolute quality, we need to compare the calculated value with
     * a threshold based on our known good estimate. Hence, we require
     * a valid f_hat value in order to determine the quality of the local
     * rate estimate.
     */
    delta_t =
        (m_stats.f_hat_accept ? to_bintime(Tf - m_data.f_local.last_update,
                                           get_value(m_data.f_hat.current))
                              : bintime::zero());
    if (f_local
        && !rate_delta_exceeded(get_value(m_data.f_local.current),
                                get_value(*f_local),
                                delta_t,
                                m_stats.f_local_accept,
                                f_local_ppm_limit)) {
        m_data.f_local.current = *f_local;
        m_data.f_local.last_update = Tf;
        m_stats.f_local_accept++;
    } else if (f_local) {
        OP_LOG(OP_LOG_WARNING,
               "f_local update rejected "
               "(%" PRIu64 " --> %" PRIu64 ", ppm = %.06f)\n",
               get_value(m_data.f_local.current).count(),
               get_value(*f_local).count(),
               get_threshold_ppm(m_stats.f_local_accept, f_local_ppm_limit));
        m_stats.f_local_reject++;
    }

    /*
     * Add the timestamp to our history with whatever the current
     * f_local value is.  A 0 value is ok.
     */
    m_history.insert(stamp, get_value(m_data.f_local.current));

    /* Drop old timestamps */
    m_history.prune_before(to_time(stamp) - max_history.count());

    /*
     * If we have a f_hat value, then we should be able to calculate
     * synchronization data and sync the local clock.
     */
    if (f_hat) { do_offset_sync(stamp); }

    return {};
}

std::optional<counter::hz> clock::frequency() const
{
    if (!m_stats.f_hat_accept) { return (std::nullopt); }

    return (get_value(m_data.f_hat.current));
}

std::optional<uint64_t> clock::frequency_error() const
{
    if (!m_stats.f_hat_accept) { return (std::nullopt); }

    return (get_error(m_data.f_hat.current));
}

std::optional<counter::hz> clock::local_frequency() const
{
    if (!m_stats.f_local_accept) { return (std::nullopt); }

    return (get_value(m_data.f_local.current));
}

std::optional<uint64_t> clock::local_frequency_error() const
{
    if (!m_stats.f_local_accept) { return (std::nullopt); }

    return (get_error(m_data.f_local.current));
}

bintime clock::offset() const { return (m_data.K); }

std::optional<bintime> clock::theta() const
{
    if (!m_stats.theta_hat_accept) { return (std::nullopt); }

    return (m_data.theta_hat.current);
}

size_t clock::nb_frequency_accept() const { return (m_stats.f_hat_accept); }

size_t clock::nb_frequency_reject() const { return (m_stats.f_hat_reject); }

size_t clock::nb_local_frequency_accept() const
{
    return (m_stats.f_local_accept);
}

size_t clock::nb_local_frequency_reject() const
{
    return (m_stats.f_local_reject);
}

size_t clock::nb_theta_accept() const { return (m_stats.theta_hat_accept); }

size_t clock::nb_theta_reject() const { return (m_stats.theta_hat_reject); }

size_t clock::nb_timestamps() const { return (m_history.size()); }

static double to_seconds(counter::ticks ticks, counter::hz hz)
{
    return (std::chrono::duration_cast<std::chrono::duration<double>>(
                to_duration(to_bintime(ticks, hz)))
                .count());
}

std::optional<double> clock::rtt_maximum() const
{
    if (m_stats.rtts.size() && m_stats.f_hat_accept) {
        return (
            to_seconds(m_stats.rtts.max(), get_value(m_data.f_hat.current)));
    }

    return (std::nullopt);
}

std::optional<double> clock::rtt_median() const
{
    if (m_stats.rtts.size() && m_stats.f_hat_accept) {
        return (
            to_seconds(static_cast<counter::ticks>(m_stats.rtts.quantile(50)),
                       get_value(m_data.f_hat.current)));
    }

    return (std::nullopt);
}

std::optional<double> clock::rtt_minimum() const
{
    if (m_stats.rtts.size() && m_stats.f_hat_accept) {
        return (
            to_seconds(m_stats.rtts.min(), get_value(m_data.f_hat.current)));
    }

    return (std::nullopt);
}

size_t clock::rtt_size() const { return (m_stats.rtts.size()); }

} // namespace openperf::timesync
