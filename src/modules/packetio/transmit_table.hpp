#ifndef _OP_PACKETIO_TRANSMIT_TABLE_HPP_
#define _OP_PACKETIO_TRANSMIT_TABLE_HPP_

#include <atomic>
#include <utility>

#include "immer/box.hpp"
#include "immer/flex_vector.hpp"

#include "packetio/immer_utils.hpp"

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
    using safe_key_type = std::tuple<uint16_t, uint16_t, size_t>;
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
    const Source* get_source(const safe_key_type&) const;

    static safe_key_type to_safe_key(const key_type&);

private:
    std::atomic<source_map*> m_sources;

    static key_type to_key(uint16_t, uint16_t, std::string_view);
};

} // namespace openperf::packetio

#endif /* _OP_PACKETIO_TRANSMIT_TABLE_HPP_ */
