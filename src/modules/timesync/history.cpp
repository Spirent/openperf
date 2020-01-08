#include "timesync/history.hpp"

namespace openperf::timesync {

static bintime to_bintime(const ntp_timestamp& src)
{
    return (bintime{.bt_sec = src.bt_sec,
                    .bt_frac = static_cast<uint64_t>(src.bt_frac) << 32});
}

static ntp_timestamp to_ntp(const bintime& src)
{
    return (ntp_timestamp{.bt_sec = static_cast<int32_t>(src.bt_sec),
                          .bt_frac = static_cast<uint32_t>(src.bt_frac >> 32)});
}

timestamp history::to_timestamp(const timesync_data& data)
{
    return (timestamp{
        .Ta = data.Ta,
        .Tb = to_bintime(data.Tb),
        .Te = to_bintime(data.Tb)
              + bintime{0, static_cast<uint64_t>(data.offset_Te) << 32},
        .Tf = data.Ta + data.offset_Tf});
}

bool history::empty() const noexcept { return (m_history.empty()); }

size_t history::size() const noexcept { return (m_history.size()); }

void history::clear() noexcept { m_history.clear(); }

bool history::contains(const timestamp& ts) const noexcept
{
    auto ntp_ts = to_ntp(ts.Tb);
    return (std::any_of(
        std::begin(m_history),
        std::end(m_history),
        [&ntp_ts](const auto& item) { return (ntp_ts == item.Tb); }));
}

void history::prune(time_t time)
{
    m_history.erase(std::begin(m_history), lower_bound(time));
}

void history::insert(const timestamp& ts, counter::hz f_local)
{
    assert(ts.Tf > ts.Ta);
    auto offset_Te = ts.Te - ts.Tb;
    if (ts.Tf - ts.Ta > std::numeric_limits<uint32_t>::max()
        || offset_Te.bt_sec != 0) {
        throw std::domain_error("timestamp interval is too big");
    }

    auto item = timesync_data{
        .Ta = ts.Ta,
        .Tb = to_ntp(ts.Tb),
        .offset_Te = static_cast<uint32_t>(offset_Te.bt_frac >> 32),
        .offset_Tf = static_cast<uint32_t>(ts.Tf - ts.Ta),
        .f_local = f_local};

    if (m_history.find(item) != m_history.end()) {
        throw std::invalid_argument("timestamp is a duplicate");
    }

    m_history.insert(item);
}

bintime history::duration() const noexcept
{
    if (m_history.empty()) return (bintime{});

    auto start = to_bintime(m_history.begin()->Tb);
    auto end = to_bintime(std::prev(m_history.end())->Tb);

    return (end - start);
}

struct timesync_data_comparator
{
    using key = history::history_container::key_type;
    bool operator()(const key& lhs, time_t rhs)
    {
        return (lhs.Tb.bt_sec < rhs);
    }

    bool operator()(time_t lhs, const key& rhs)
    {
        return (lhs < rhs.Tb.bt_sec);
    }
};

history::history_container::iterator history::lower_bound(time_t time) const
{
    return (std::lower_bound(std::begin(m_history),
                             std::end(m_history),
                             time,
                             timesync_data_comparator()));
}

history::history_container::iterator history::upper_bound(time_t time) const
{
    return (std::upper_bound(std::begin(m_history),
                             std::end(m_history),
                             time,
                             timesync_data_comparator()));
}

} // namespace openperf::timesync
