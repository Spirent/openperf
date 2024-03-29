#include "core/op_log.h"
#include "packetio/transmit_table.hpp"
#include "utils/hash_combine.hpp"

namespace openperf::packetio {

template <typename Source> transmit_table<Source>::transmit_table()
{
    m_sources.store(new source_map{});
}

template <typename Source> transmit_table<Source>::~transmit_table()
{
    delete m_sources.exchange(nullptr);
}

template <typename Source>
typename transmit_table<Source>::key_type transmit_table<Source>::to_key(
    uint16_t port_idx, uint16_t queue_idx, std::string_view source_id)
{
    return {port_idx, queue_idx, std::string(source_id)};
}

template <typename Source>
typename transmit_table<Source>::safe_key_type
transmit_table<Source>::to_safe_key(
    const typename transmit_table<Source>::key_type& key)
{
    return {std::get<key_port_idx>(key),
            std::get<key_queue_idx>(key),
            std::hash<std::string>{}(std::get<key_id_idx>(key))};
}

/*
 * The various binary search algorithms, e.g. equal_range, lower_bound, etc.,
 * need to make the "less than" comparison between keys and values in any order.
 * To facilitate this, We make use of two comparison structs.  The first
 * compares full keys.  The second only compares port/queue indexes.
 */
template <typename Source> struct key_comparator
{
    bool operator()(const typename transmit_table<Source>::key_type& left,
                    const typename transmit_table<Source>::value_type& right)
    {
        return (left < right.first);
    }

    bool operator()(const typename transmit_table<Source>::value_type& left,
                    const typename transmit_table<Source>::key_type& right)
    {
        return (left.first < right);
    }
};

template <typename Source, typename T>
static inline std::pair<uint16_t, uint16_t>
to_port_queue_pair(const std::tuple<uint16_t, uint16_t, T>& key)
{
    static constexpr auto key_port_idx = transmit_table<Source>::key_port_idx;
    static constexpr auto key_queue_idx = transmit_table<Source>::key_queue_idx;

    return (std::make_pair(std::get<key_port_idx>(key),
                           std::get<key_queue_idx>(key)));
}

template <typename Source> struct safe_key_comparator
{
    bool operator()(const typename transmit_table<Source>::safe_key_type& left,
                    const typename transmit_table<Source>::value_type& right)
    {
        return (to_port_queue_pair<Source>(left)
                < to_port_queue_pair<Source>(right.first));
    }

    bool operator()(const typename transmit_table<Source>::value_type& left,
                    const typename transmit_table<Source>::safe_key_type& right)
    {
        return (to_port_queue_pair<Source>(left.first)
                < to_port_queue_pair<Source>(right));
    }
};

template <typename Source> struct port_queue_comparator
{
    bool operator()(const typename transmit_table<Source>::key_type& left,
                    const typename transmit_table<Source>::value_type& right)
    {
        return (to_port_queue_pair<Source>(left)
                < to_port_queue_pair<Source>(right.first));
    }

    bool operator()(const typename transmit_table<Source>::value_type& left,
                    const typename transmit_table<Source>::key_type& right)
    {
        return (to_port_queue_pair<Source>(left.first)
                < to_port_queue_pair<Source>(right));
    }
};

template <typename Source>
typename transmit_table<Source>::source_map*
transmit_table<Source>::insert_source(uint16_t port_idx,
                                      uint16_t queue_idx,
                                      Source source)
{
    auto key = to_key(port_idx, queue_idx, source.id());
    auto original = m_sources.load(std::memory_order_consume);
    auto precursor = std::lower_bound(
        original->begin(), original->end(), key, key_comparator<Source>{});
    auto updated = new source_map{
        std::move(original->insert(std::distance(original->begin(), precursor),
                                   std::make_pair(key, std::move(source))))};
    return (m_sources.exchange(updated, std::memory_order_release));
}

template <typename Source>
typename transmit_table<Source>::source_map*
transmit_table<Source>::remove_source(uint16_t port_idx,
                                      uint16_t queue_idx,
                                      std::string_view source_id)
{
    auto key = to_key(port_idx, queue_idx, source_id);
    auto original = m_sources.load(std::memory_order_consume);
    auto range = std::equal_range(
        original->begin(), original->end(), key, key_comparator<Source>{});
    auto found = std::find_if(range.first, range.second, [&](const auto& item) {
        return (item.get().first == key);
    });
    if (found == range.second) return (nullptr);
    auto updated = new source_map{
        std::move(original->erase(std::distance(original->begin(), found)))};
    return (m_sources.exchange(updated, std::memory_order_release));
}

template <typename Source>
std::pair<typename transmit_table<Source>::source_map::iterator,
          typename transmit_table<Source>::source_map::iterator>
transmit_table<Source>::get_sources(uint16_t port_idx) const
{
    auto lo_key = to_key(port_idx, 0, "");
    auto hi_key = to_key(port_idx, std::numeric_limits<uint16_t>::max(), "");
    auto map = m_sources.load(std::memory_order_consume);
    return (std::make_pair(
        std::lower_bound(
            map->begin(), map->end(), lo_key, port_queue_comparator<Source>{}),
        std::upper_bound(map->begin(),
                         map->end(),
                         hi_key,
                         port_queue_comparator<Source>{})));
}

template <typename Source>
std::pair<typename transmit_table<Source>::source_map::iterator,
          typename transmit_table<Source>::source_map::iterator>
transmit_table<Source>::get_sources(uint16_t port_idx, uint16_t queue_idx) const
{
    auto key = to_key(port_idx, queue_idx, "");
    auto map = m_sources.load(std::memory_order_consume);
    return (std::equal_range(
        map->begin(), map->end(), key, port_queue_comparator<Source>{}));
}

template <typename Source>
const Source* transmit_table<Source>::get_source(
    const transmit_table<Source>::key_type& key) const
{
    auto map = m_sources.load(std::memory_order_consume);
    auto range = std::equal_range(
        map->begin(), map->end(), key, key_comparator<Source>{});
    assert(std::distance(range.first, range.second) <= 1);
    return (std::distance(range.first, range.second) == 1
                ? std::addressof(range.first->get().second)
                : nullptr);
}

template <typename Source>
const Source* transmit_table<Source>::get_source(
    const transmit_table<Source>::safe_key_type& key) const
{
    auto map = m_sources.load(std::memory_order_consume);
    /*
     * XXX: safe_key_comparator only compares port/queue indexes.
     * We still need to explicitly find the matching hash value.
     */
    auto range = std::equal_range(
        map->begin(), map->end(), key, safe_key_comparator<Source>{});
    const auto hash = std::get<key_id_idx>(key);
    auto cursor =
        std::find_if(range.first, range.second, [&](const auto& item) {
            return (std::hash<std::string>{}(std::get<key_id_idx>(item->first))
                    == hash);
        });
    return (cursor == range.second ? nullptr
                                   : std::addressof(cursor->get().second));
}

} // namespace openperf::packetio

/* There is no standard hash for an array, so we need to provide one */
namespace std {
template <size_t N> struct hash<std::array<char, N>>
{
    size_t operator()(const std::array<char, N>& array) const
    {
        std::hash<std::string_view> hasher;
        return (hasher(std::string_view{array.data()}));
    }
};

} // namespace std
