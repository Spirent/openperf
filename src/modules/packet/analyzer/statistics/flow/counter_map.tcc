#include "packet/analyzer/statistics/flow/counter_map.hpp"
#include "utils/hash_combine.hpp"

namespace openperf::packet::analyzer::statistics::flow {

template <typename FlowStats> counter_map<FlowStats>::counter_map()
{
    m_stats.store(new map{});
}

template <typename FlowStats> counter_map<FlowStats>::~counter_map()
{
    delete m_stats.exchange(nullptr);
}

template <typename FlowStats>
typename counter_map<FlowStats>::map*
counter_map<FlowStats>::insert(uint32_t rss_hash,
                               std::optional<uint32_t> stream_id,
                               const FlowStats& stats)
{
    auto original = m_stats.load(std::memory_order_consume);
    auto key = std::make_pair(rss_hash, stream_id);
    auto updated = new map{
        std::move(original->insert(std::make_pair(key, std::move(stats))))};
    return (m_stats.exchange(updated, std::memory_order_release));
}

template <typename FlowStats>
typename counter_map<FlowStats>::map* counter_map<FlowStats>::insert(
    uint32_t rss_hash, std::optional<uint32_t> stream_id, FlowStats&& stats)
{
    auto original = m_stats.load(std::memory_order_consume);
    auto key = std::make_pair(rss_hash, stream_id);
    auto updated = new map{std::move(
        original->insert(std::make_pair(key, std::forward<FlowStats>(stats))))};
    return (m_stats.exchange(updated, std::memory_order_release));
}

template <typename FlowStats>
const FlowStats*
counter_map<FlowStats>::find(uint32_t rss_hash,
                             std::optional<uint32_t> stream_id) const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->find({rss_hash, stream_id}));
}

template <typename FlowStats>
const FlowStats*
counter_map<FlowStats>::find(const counter_map<FlowStats>::key_type& key) const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->find(key));
}

template <typename FlowStats>
typename counter_map<FlowStats>::map::iterator
counter_map<FlowStats>::begin() const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->begin());
}

template <typename FlowStats>
typename counter_map<FlowStats>::map::iterator
counter_map<FlowStats>::end() const
{
    auto map = m_stats.load(std::memory_order_relaxed);
    return (map->end());
}

} // namespace openperf::packet::analyzer::statistics::flow
