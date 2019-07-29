#include "core/icp_log.h"
#include "packetio/transmit_table.h"
#include "utils/hash_combine.h"

namespace icp::packetio {

template <typename Source>
transmit_table<Source>::transmit_table()
{
    m_sources.store(new source_map{});
}

template <typename Source>
transmit_table<Source>::~transmit_table()
{
    delete m_sources.exchange(nullptr);
}

template <typename Source>
typename transmit_table<Source>::source_key
transmit_table<Source>::to_key(uint16_t port_idx, uint16_t queue_idx, std::string_view source_id)
{
    if (source_id.length() > key_buffer_length) {
        ICP_LOG(ICP_LOG_WARNING, "Truncating source id = %.*s to %zu characters\n",
                static_cast<int>(source_id.length()), source_id.data(),
                key_buffer_length);
    }

    auto key = source_key{port_idx, queue_idx, {}};
    auto& buffer = std::get<key_buffer_idx>(key);
    std::copy(source_id.data(),
              source_id.data() + std::min(source_id.length(), key_buffer_length),
              buffer.data());

    return (key);
}

template <typename Source>
typename transmit_table<Source>::source_map*
transmit_table<Source>::insert_source(uint16_t port_idx, uint16_t queue_idx,
                                      Source source)
{
    auto key = to_key(port_idx, queue_idx, source.id());
    auto original = m_sources.load(std::memory_order_consume);
    auto updated = new source_map{original->set(key, source)};
    return (m_sources.exchange(updated, std::memory_order_release));
}

template <typename Source>
typename transmit_table<Source>::source_map*
transmit_table<Source>::remove_source(uint16_t port_idx, uint16_t queue_idx,
                                      Source source)
{
    auto key = to_key(port_idx, queue_idx, source.id());
    auto original = m_sources.load(std::memory_order_consume);
    auto updated = new source_map{original->erase(key)};
    return (m_sources.exchange(updated, std::memory_order_release));
}

/*
 * The various binary search algorithms, e.g. equal_range, lower_bound, etc.,
 * need to make the "less than" comparison between keys and values in any order.
 */
template <typename Source>
struct source_map_comparator
{
    static constexpr auto key_port_idx  = transmit_table<Source>::key_port_idx;
    static constexpr auto key_queue_idx = transmit_table<Source>::key_queue_idx;

    bool operator()(const typename transmit_table<Source>::source_map::value_type& left,
                    const typename transmit_table<Source>::source_map::key_type& right)
    {
        /* Turn port/queue indexes into a number and compare them */
        auto left_id = (static_cast<uint32_t>(std::get<key_port_idx>(left.first)) << 16
                        | std::get<key_queue_idx>(left.first));
        auto right_id = (static_cast<uint32_t>(std::get<key_port_idx>(right)) << 16
                         | std::get<key_queue_idx>(right));

        return (left_id < right_id);
    }

    bool operator()(const typename transmit_table<Source>::source_map::key_type& left,
                    const typename transmit_table<Source>::source_map::value_type& right)
    {
        /* Same deal as above, but for flipped types */
        auto left_id = (static_cast<uint32_t>(std::get<key_port_idx>(left)) << 16
                        | std::get<key_queue_idx>(left));
        auto right_id = (static_cast<uint32_t>(std::get<key_port_idx>(right.first)) << 16
                         | std::get<key_queue_idx>(right.first));

        return (left_id < right_id);
    }
};

template <typename Source>
std::pair<typename transmit_table<Source>::source_map::iterator,
          typename transmit_table<Source>::source_map::iterator>
transmit_table<Source>::get_sources(uint16_t port_idx) const
{
    auto lo_key = to_key(port_idx, 0, "");
    auto hi_key = to_key(port_idx, std::numeric_limits<uint16_t>::max(), "");
    auto map = m_sources.load(std::memory_order_consume);
    return (std::make_pair(std::lower_bound(map->begin(), map->end(), lo_key,
                                            source_map_comparator<Source>{}),
                           std::upper_bound(map->begin(), map->end(), hi_key,
                                            source_map_comparator<Source>{})));
}

template <typename Source>
std::pair<typename transmit_table<Source>::source_map::iterator,
          typename transmit_table<Source>::source_map::iterator>
transmit_table<Source>::get_sources(uint16_t port_idx, uint16_t queue_idx) const
{
    auto key = to_key(port_idx, queue_idx, "");
    auto map = m_sources.load(std::memory_order_consume);
    return (std::equal_range(map->begin(), map->end(), key,
                             source_map_comparator<Source>{}));
}

template <typename Source>
const Source* transmit_table<Source>::get_source(uint16_t port_idx, uint16_t queue_idx,
                                                 std::string_view source_id) const
{
    auto key = to_key(port_idx, queue_idx, source_id);
    auto map = m_sources.load(std::memory_order_consume);
    return (map->find(key));
}

}

/* There is no standard hash for an array, so we need to provide one */
namespace std {
template <size_t N>
struct hash<std::array<char, N>>
{
    size_t operator()(const std::array<char, N>& array) const
    {
        std::hash<std::string_view> hasher;
        return (hasher(std::string_view{array.data()}));
    }
};

}
