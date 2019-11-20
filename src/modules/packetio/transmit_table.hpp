#ifndef _OP_PACKETIO_TRANSMIT_TABLE_HPP_
#define _OP_PACKETIO_TRANSMIT_TABLE_HPP_

#include <atomic>
#include <utility>

#include "immer/flex_vector.hpp"

namespace openperf::packetio {

template <typename Source>
class transmit_table
{
    /*
     * XXX: 64 is a biased, yet arbitrary constant.
     * It is probably the host machines cache line size, which keeps our
     * key/value pair limited to the size of a cache line.  Conveniently,
     * it also gives us sufficient room to store a stringified UUID, which
     * is the default id for sources.
     */
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
    using source_pair = std::pair<source_key, Source>;
    using source_map = immer::flex_vector<source_pair>;

    source_map* insert_source(uint16_t port_idx, uint16_t queue_idx,
                              Source source);
    source_map* remove_source(uint16_t port_idx, uint16_t queue_idx,
                              std::string_view source_id);

    std::pair<typename source_map::iterator,
              typename source_map::iterator>
    get_sources(uint16_t port_idx) const;

    std::pair<typename source_map::iterator,
              typename source_map::iterator>
    get_sources(uint16_t port_idx, uint16_t queue_idx) const;

    const Source* get_source(const source_key& key) const;

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

#endif /* _OP_PACKETIO_TRANSMIT_TABLE_HPP_ */
