#ifndef _OP_PACKETIO_TRANSMIT_TABLE_HPP_
#define _OP_PACKETIO_TRANSMIT_TABLE_HPP_

#include <atomic>
#include <utility>

#include "immer/box.hpp"
#include "immer/flex_vector.hpp"

namespace openperf::packetio {

template <typename Source> class transmit_table
{
public:
    transmit_table();
    ~transmit_table();

    static constexpr auto key_port_idx = 0;
    static constexpr auto key_queue_idx = 1;
    static constexpr auto key_id_idx = 2;

    using key_type = std::tuple<uint16_t, uint16_t, std::string>;
    using noalloc_key_type = std::tuple<uint16_t, uint16_t, std::string_view>;
    using mapped_type = Source;
    using value_type = std::pair<key_type, mapped_type>;
    using source_entry = immer::box<value_type>;
    using source_map = immer::flex_vector<source_entry>;

    source_map*
    insert_source(uint16_t port_idx, uint16_t queue_idx, Source source);
    source_map* remove_source(uint16_t port_idx,
                              uint16_t queue_idx,
                              std::string_view source_id);

    std::pair<typename source_map::iterator, typename source_map::iterator>
    get_sources(uint16_t port_idx) const;

    std::pair<typename source_map::iterator, typename source_map::iterator>
    get_sources(uint16_t port_idx, uint16_t queue_idx) const;

    const Source* get_source(const key_type&) const;
    const Source* get_source(const noalloc_key_type&) const;

private:
    std::atomic<source_map*> m_sources;

    static key_type to_key(uint16_t, uint16_t, std::string_view);
};

} // namespace openperf::packetio

/*
 * Provide overloads to allow range based iteration of the pairs returned
 * by get_sources().  Since the lookup is done via the namespace of
 * the type, we need to add these functions to the 'immer' namespace.
 */
namespace immer {

template <typename Iterator>
Iterator begin(const std::pair<Iterator, Iterator>& range)
{
    return (range.first);
}

template <typename Iterator>
Iterator end(const std::pair<Iterator, Iterator>& range)
{
    return (range.second);
}

} // namespace immer

#endif /* _OP_PACKETIO_TRANSMIT_TABLE_HPP_ */
