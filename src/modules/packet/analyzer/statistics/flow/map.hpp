#ifndef _OP_ANALYZER_STATISTICS_FLOW_MAP_HPP_
#define _OP_ANALYZER_STATISTICS_FLOW_MAP_HPP_

#include <atomic>
#include <cstddef>
#include <optional>
#include <tuple>
#include <variant>

#include "immer/map.hpp"

namespace openperf::packet::analyzer::statistics::flow {

template <typename FlowStats> class map
{
public:
    map();
    ~map();

    using key_type = std::pair<uint32_t, std::optional<uint32_t>>;
    using value_type = FlowStats;

    using map_type = immer::map<key_type, value_type>;

    map_type* insert(uint32_t rss_hash,
                     std::optional<uint32_t> stream_id,
                     const value_type& stats);

    map_type* insert(uint32_t rss_hash,
                     std::optional<uint32_t> stream_id,
                     value_type&& stats);

    const value_type*
    find(uint32_t rss_hash,
         std::optional<uint32_t> stream_id = std::nullopt) const;

    const value_type* find(const key_type& key) const;

    typename map_type::iterator begin() const;
    typename map_type::iterator end() const;

private:
    std::atomic<map_type*> m_map;
};

} // namespace openperf::packet::analyzer::statistics::flow

#endif /* _OP_ANALYZER_STATISTICS_FLOW_MAP_HPP_ */
