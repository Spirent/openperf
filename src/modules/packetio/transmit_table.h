#ifndef _ICP_PACKETIO_TRANSMIT_TABLE_H_
#define _ICP_PACKETIO_TRANSMIT_TABLE_H_

#include <atomic>
#include <utility>

#include "immer/map.hpp"

namespace icp::packetio {

template <typename Source>
class transmit_table
{
    static constexpr auto key_buffer_length =
        64 - (2 * sizeof(uint16_t)) - sizeof(Source);
public:
    transmit_table();
    ~transmit_table();

    static constexpr auto key_port_idx   = 0;
    static constexpr auto key_queue_idx  = 1;
    static constexpr auto key_buffer_idx = 2;

    using source_key = std::tuple<uint16_t, uint16_t,
                                  std::array<char, key_buffer_length>>;
    using source_map = immer::map<source_key, Source>;

    source_map* insert_source(uint16_t port_idx, uint16_t queue_idx,
                              Source source);
    source_map* remove_source(uint16_t port_idx, uint16_t queue_idx,
                              Source source);

    std::pair<typename source_map::iterator,
              typename source_map::iterator>
    get_sources(uint16_t port_idx) const;

    std::pair<typename source_map::iterator,
              typename source_map::iterator>
    get_sources(uint16_t port_idx, uint16_t queue_idx) const;

    const Source* get_source(uint16_t port_idx, uint16_t queue_idx,
                             std::string_view source_id) const;

private:
    std::atomic<source_map*> m_sources;

    static source_key to_key(uint16_t, uint16_t, std::string_view);

    static_assert(sizeof(source_key) + sizeof(Source) == 64);
};

}

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

}

#endif /* _ICP_PACKETIO_TRANSMIT_TABLE_H_ */
