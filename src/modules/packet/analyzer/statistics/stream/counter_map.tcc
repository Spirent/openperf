#include "packet/analyzer/statistics/stream/counter_map.hpp"
#include "utils/hash_combine.hpp"

namespace openperf::packet::analyzer::statistics::stream {

template <typename StreamStats> counter_map<StreamStats>::counter_map()
{
    m_stats.store(new map{});
}

template <typename StreamStats> counter_map<StreamStats>::~counter_map()
{
    delete m_stats.exchange(nullptr);
}

template <typename StreamStats>
typename counter_map<StreamStats>::map*
counter_map<StreamStats>::insert(uint32_t rss_hash,
                                 std::optional<uint32_t> stream_id,
                                 const StreamStats& stats)
{
    auto original = m_stats.load(std::memory_order_consume);
    auto key = std::make_pair(rss_hash, stream_id);
    auto updated = new map{
        std::move(original->insert(std::make_pair(key, std::move(stats))))};
    return (m_stats.exchange(updated, std::memory_order_release));
}

template <typename StreamStats>
typename counter_map<StreamStats>::map* counter_map<StreamStats>::insert(
    uint32_t rss_hash, std::optional<uint32_t> stream_id, StreamStats&& stats)
{
    auto original = m_stats.load(std::memory_order_consume);
    auto key = std::make_pair(rss_hash, stream_id);
    auto updated = new map{std::move(original->insert(
        std::make_pair(key, std::forward<StreamStats>(stats))))};
    return (m_stats.exchange(updated, std::memory_order_release));
}

template <typename StreamStats>
const StreamStats*
counter_map<StreamStats>::find(uint32_t rss_hash,
                               std::optional<uint32_t> stream_id) const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->find({rss_hash, stream_id}));
}

template <typename StreamStats>
const StreamStats* counter_map<StreamStats>::find(
    const counter_map<StreamStats>::key_type& key) const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->find(key));
}

template <typename StreamStats>
typename counter_map<StreamStats>::map::iterator
counter_map<StreamStats>::begin() const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->begin());
}

template <typename StreamStats>
typename counter_map<StreamStats>::map::iterator
counter_map<StreamStats>::end() const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->end());
}

} // namespace openperf::packet::analyzer::statistics::stream
