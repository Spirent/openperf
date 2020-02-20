#ifndef _OP_ANALYZER_STATISTICS_STREAM_COUNTER_MAP_HPP_
#define _OP_ANALYZER_STATISTICS_STREAM_COUNTER_MAP_HPP_

#include <atomic>
#include <cstddef>
#include <optional>
#include <tuple>
#include <variant>

#include "immer/map.hpp"

namespace openperf::packet::analyzer::statistics::stream {

template <typename StreamStats> class counter_map
{
public:
    counter_map();
    ~counter_map();

    using key_type = std::pair<uint32_t, std::optional<uint32_t>>;
    using value_type = StreamStats;

    using map = immer::map<key_type, value_type>;

    map* insert(uint32_t rss_hash,
                std::optional<uint32_t> stream_id,
                const value_type& stats);

    map* insert(uint32_t rss_hash,
                std::optional<uint32_t> stream_id,
                value_type&& stats);

    const value_type*
    find(uint32_t rss_hash,
         std::optional<uint32_t> stream_id = std::nullopt) const;

    const value_type* find(const key_type& key) const;

    typename map::iterator begin() const;
    typename map::iterator end() const;

private:
    std::atomic<map*> m_stats;
};

} // namespace openperf::packet::analyzer::statistics::stream

#endif /* _OP_ANALYZER_STATISTICS_STREAM_COUNTER_MAP_HPP_ */
