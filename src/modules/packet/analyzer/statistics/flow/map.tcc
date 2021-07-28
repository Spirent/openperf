#include "packet/analyzer/statistics/flow/map.hpp"
#include "utils/hash_combine.hpp"

namespace openperf::packet::analyzer::statistics::flow {

template <typename FlowStats> map<FlowStats>::map()
{
    m_map.store(new map_type{});
}

template <typename FlowStats> map<FlowStats>::~map()
{
    delete m_map.exchange(nullptr);
}

template <typename FlowStats>
typename map<FlowStats>::map_type*
map<FlowStats>::insert(const key_type& key, const FlowStats& stats)
{
    auto* original = m_map.load(std::memory_order_consume);
    auto* updated = new map_type{
        std::move(original->insert(std::make_pair(key, std::move(stats))))};
    return (m_map.exchange(updated, std::memory_order_release));
}

template <typename FlowStats>
typename map<FlowStats>::map_type* map<FlowStats>::insert(const key_type& key,
                                                          FlowStats&& stats)
{
    auto* original = m_map.load(std::memory_order_consume);
    auto* updated = new map_type{std::move(
        original->insert(std::make_pair(key, std::forward<FlowStats>(stats))))};
    return (m_map.exchange(updated, std::memory_order_release));
}

template <typename FlowStats>
const FlowStats* map<FlowStats>::find(uint32_t rss_hash,
                                      uint32_t stream_id) const
{
    auto* map = m_map.load(std::memory_order_relaxed);
    return (map->find({rss_hash, stream_id}));
}

template <typename FlowStats>
const FlowStats* map<FlowStats>::find(const map<FlowStats>::key_type& key) const
{
    auto* map = m_map.load(std::memory_order_relaxed);
    return (map->find(key));
}

template <typename FlowStats>
const FlowStats& map<FlowStats>::at(const map<FlowStats>::key_type& key) const
{
    auto* map = m_map.load(std::memory_order_relaxed);
    return (map->at(key));
}

template <typename FlowStats>
typename map<FlowStats>::map_type::iterator map<FlowStats>::begin() const
{
    auto* map = m_map.load(std::memory_order_relaxed);
    return (map->begin());
}

template <typename FlowStats>
typename map<FlowStats>::map_type::iterator map<FlowStats>::end() const
{
    auto* map = m_map.load(std::memory_order_relaxed);
    return (map->end());
}

} // namespace openperf::packet::analyzer::statistics::flow
