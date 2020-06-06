#include "packet/generator/api.hpp"
#include "packet/generator/traffic/sequence.hpp"

namespace openperf::packet::generator::traffic {

namespace impl {

template <typename Template, template <typename T> class Strategy>
class sequence_generator
{
public:
    /*
     * This sequence generator combines multiple weighted packet templates
     * into a single sequence of packets. The generated output consists of
     * a sequence of index pairs:
     * - the first item indicates the packet template
     * - the second item indicates the index in the packet sequence
     */
    using view_type = std::pair<uint16_t, uint16_t>;
    using iterator = view_iterator<sequence_generator>;
    using const_iterator = const iterator;

    sequence_generator(const definition_container& definitions)
    {
        m_template_sizes.reserve(definitions.size());
        m_template_scalars.reserve(definitions.size());

        std::transform(std::begin(definitions),
                       std::end(definitions),
                       std::back_inserter(m_template_sizes),
                       [](auto&& item) {
                           const auto size = std::get<Template>(item).size();
                           /*
                            * XXX: this limitation is arbitrary, but done in the
                            * interest of keeping the generated sequence as
                            * small as possible.
                            */
                           assert(size <= std::numeric_limits<uint16_t>::max());
                           return (size);
                       });

        std::transform(std::begin(definitions),
                       std::end(definitions),
                       std::back_inserter(m_template_scalars),
                       [](auto&& item) { return (std::get<2>(item)); });
    }

    size_t size() const
    {
        return (static_cast<const Strategy<Template>&>(*this).get_size());
    }

    view_type operator[](size_t idx) const
    {
        return (static_cast<const Strategy<Template>&>(*this).get_indexes(idx));
    }

    iterator begin() { return (iterator(*this)); };
    const_iterator begin() const { return (iterator(*this)); };

    iterator end() { return (iterator(*this, size())); };
    const_iterator end() const { return (iterator(*this, size())); }

protected:
    using scalar_container = std::vector<unsigned>;
    const scalar_container& template_sizes() const
    {
        return (m_template_sizes);
    }
    const scalar_container& template_scalars() const
    {
        return (m_template_scalars);
    }

private:
    scalar_container m_template_sizes;
    scalar_container m_template_scalars;
};

template <typename Template>
class round_robin final : public sequence_generator<Template, round_robin>
{
    using parent = sequence_generator<Template, round_robin>;
    typename parent::scalar_container m_weight_sums;

public:
    round_robin(const definition_container& definitions)
        : parent::sequence_generator(definitions)
    {
        m_weight_sums.reserve(parent::template_scalars().size());
        std::partial_sum(std::begin(parent::template_scalars()),
                         std::end(parent::template_scalars()),
                         std::back_inserter(m_weight_sums));
    }

    size_t get_size() const
    {
        auto lcm = std::accumulate(
            std::begin(parent::template_sizes()),
            std::end(parent::template_sizes()),
            1UL,
            [](size_t lhs, size_t rhs) { return (std::lcm(lhs, rhs)); });

        return (lcm * m_weight_sums.back());
    }

    typename parent::view_type get_indexes(size_t idx) const
    {
        const auto sum = m_weight_sums.back();
        auto quot = idx / sum;
        auto rem = idx % sum;

        auto tmp_idx = std::distance(std::begin(m_weight_sums),
                                     std::upper_bound(std::begin(m_weight_sums),
                                                      std::end(m_weight_sums),
                                                      rem));

        auto offset = quot * sum
                      + (tmp_idx == 0 ? 0 : m_weight_sums[tmp_idx - 1])
                      - quot * parent::template_scalars()[tmp_idx];

        assert(static_cast<size_t>(offset) <= idx);

        return {tmp_idx, (idx - offset) % parent::template_sizes()[tmp_idx]};
    }
};

template <typename Template>
class sequential final : public sequence_generator<Template, sequential>
{
    using parent = sequence_generator<Template, sequential>;
    typename parent::scalar_container m_lengths;
    typename parent::scalar_container m_lengths_sums;

public:
    sequential(const definition_container& definitions)
        : parent::sequence_generator(definitions)
    {
        m_lengths.reserve(parent::template_scalars().size());
        m_lengths_sums.reserve(parent::template_scalars().size());

        std::transform(std::begin(parent::template_scalars()),
                       std::end(parent::template_scalars()),
                       std::begin(parent::template_sizes()),
                       std::back_inserter(m_lengths),
                       [](unsigned lhs, unsigned rhs) { return (lhs * rhs); });

        std::partial_sum(std::begin(m_lengths),
                         std::end(m_lengths),
                         std::back_inserter(m_lengths_sums));
    }

    size_t get_size() const { return (m_lengths_sums.back()); }

    typename parent::view_type get_indexes(size_t idx) const
    {
        const auto sum = m_lengths_sums.back();
        auto quot = idx / sum;
        auto rem = idx % sum;

        auto tmp_idx = std::distance(
            std::begin(m_lengths_sums),
            std::upper_bound(
                std::begin(m_lengths_sums), std::end(m_lengths_sums), rem));

        auto offset = quot * sum
                      + (tmp_idx == 0 ? 0 : m_lengths_sums[tmp_idx - 1])
                      - quot * m_lengths[tmp_idx];

        assert(static_cast<size_t>(offset) <= idx);

        return {tmp_idx, (idx - offset) % parent::template_sizes()[tmp_idx]};
    }
};

} // namespace impl

sequence sequence::round_robin_sequence(definition_container&& definitions)
{
    return (sequence(std::forward<definition_container>(definitions),
                     order_type::round_robin));
}

sequence sequence::sequential_sequence(definition_container&& definitions)
{
    return (sequence(std::forward<definition_container>(definitions),
                     order_type::sequential));
}

template <typename InputIt1,
          typename InputIt2,
          typename InputIt3,
          typename T,
          typename BinaryOp1,
          typename BinaryOp2>
T fancy_inner_product(InputIt1 cursor1,
                      InputIt1 end,
                      InputIt2 cursor2,
                      InputIt3 cursor3,
                      T sum,
                      BinaryOp1 op1,
                      BinaryOp2 op2)
{
    while (cursor1 != end) {
        sum += op1(*cursor1, op2(*cursor2, *cursor3));
        cursor1++;
        cursor2++;
        cursor3++;
    }

    return (sum);
}

sequence::sequence(definition_container&& definitions, order_type order)
{
    m_definitions.reserve(definitions.size());
    std::transform(
        std::begin(definitions),
        std::end(definitions),
        std::back_inserter(m_definitions),
        [](auto&& item) { return (std::forward<decltype(item)>(item)); });

    const auto& packet_templates = m_definitions.get<0>();
    m_flow_offsets.reserve(definitions.size());
    auto offset = 0U;
    std::transform(std::begin(packet_templates),
                   std::end(packet_templates),
                   std::back_inserter(m_flow_offsets),
                   [&](const auto& pt) {
                       auto tmp = offset;
                       offset += pt.size();
                       return (tmp);
                   });

    /*
     * Both the sequence of packets and size depend on our order type.
     * We consider the length of each definition to be the
     * lcm(# packets, #lengths).
     */
    const auto& length_templates = m_definitions.get<1>();
    const auto& weights = m_definitions.get<2>();

    if (order == order_type::round_robin) {
        auto rr_packet_gen = impl::round_robin<packet_template>(definitions);
        m_packet_indexes.reserve(rr_packet_gen.size());
        std::copy(std::begin(rr_packet_gen),
                  std::end(rr_packet_gen),
                  std::back_inserter(m_packet_indexes));

        /*
         * Definitions with weights > 1 might have repeated packets in the
         * sequence before every packet has been generated. Use the lcm of
         * all packet/length template lcm's to determine the minimum number
         * of packets in the sequence.
         */
        auto flow_lcm =
            std::inner_product(std::begin(packet_templates),
                               std::end(packet_templates),
                               std::begin(length_templates),
                               1UL,
                               std::lcm<uint64_t, uint64_t>,
                               [](const auto& lhs, const auto& rhs) {
                                   return (std::lcm(lhs.size(), rhs.size()));
                               });

        /* Then, scale by weight */
        m_size = flow_lcm
                 * std::accumulate(std::begin(weights), std::end(weights), 0UL);

    } else {
        auto seq_packet_gen = impl::sequential<packet_template>(definitions);
        m_packet_indexes.reserve(seq_packet_gen.size());
        std::copy(std::begin(seq_packet_gen),
                  std::end(seq_packet_gen),
                  std::back_inserter(m_packet_indexes));

        /*
         * Sequential generation iterates over each packet template in turn,
         * so the total size is just the sum of weight * definition length.
         * Unfortunately, there is no std algorithm for that, so use a custom
         * one.
         */
        m_size =
            fancy_inner_product(std::begin(weights),
                                std::end(weights),
                                std::begin(packet_templates),
                                std::begin(length_templates),
                                0UL,
                                std::multiplies<size_t>{},
                                [](const auto& lhs, const auto& rhs) {
                                    return (std::lcm(lhs.size(), rhs.size()));
                                });
    }

    /*
     * Finally, generate an index sequence for the lengths.
     * The scratch keeps track of our offset into each definition.
     */
    auto scratch = std::vector<size_t>(definitions.size());
    m_length_indexes.reserve(m_size);
    for (size_t i = 0; i < m_size; i++) {
        auto pkt_key = m_packet_indexes[i % m_packet_indexes.size()];
        m_length_indexes.emplace_back(
            pkt_key.first,
            scratch[pkt_key.first] % length_templates[pkt_key.first].size());
        scratch[pkt_key.first]++;
    }
}

uint16_t sequence::max_packet_length() const
{
    const auto& length_templates = m_definitions.get<1>();
    return (std::accumulate(std::begin(length_templates),
                            std::end(length_templates),
                            0U,
                            [](uint16_t lhs, const auto& rhs) {
                                return (std::max(lhs, rhs.max_packet_length()));
                            }));
}

template <typename InputIt, typename Size, typename T, typename BinaryOp>
T ring_accumulate_n(
    InputIt first, InputIt last, InputIt cursor, Size n, T sum, BinaryOp&& op)
{
    auto idx = 0U;
    while (idx++ < n) {
        sum = op(std::move(sum), *cursor++);
        if (cursor == last) { cursor = first; };
    }

    return (sum);
}

size_t sequence::sum_packet_lengths() const
{
    const auto& length_templates = m_definitions.get<1>();

    return (ring_accumulate_n(
        std::begin(m_packet_indexes),
        std::end(m_packet_indexes),
        std::begin(m_packet_indexes),
        size(),
        0UL,
        [&](auto&& lhs, const auto& rhs) {
            auto len_idx = rhs.second % length_templates[rhs.first].size();
            return (lhs + length_templates[rhs.first][len_idx]);
        }));
}

size_t sequence::sum_packet_lengths(size_t idx) const
{
    const auto& length_templates = m_definitions.get<1>();
    auto q = lldiv(idx, m_packet_indexes.size());

    auto sum =
        std::accumulate(std::begin(m_packet_indexes),
                        std::begin(m_packet_indexes) + q.rem,
                        0UL,
                        [&](size_t lhs, const auto& rhs) {
                            auto len_idx =
                                rhs.second % length_templates[rhs.first].size();
                            return (lhs + length_templates[rhs.first][len_idx]);
                        });

    if (q.quot) { sum += q.quot * sum_packet_lengths(); }

    return (sum);
}

template <typename InputIt, typename T, typename FilterOp, typename BinaryOp>
T filter_accumulate(
    InputIt cursor, InputIt last, T total, FilterOp&& filter, BinaryOp&& op)
{
    while (cursor != last) {
        if (filter(*cursor)) { total = op(std::move(total), *cursor); }
        ++cursor;
    }

    return (total);
}

size_t sequence::sum_flow_packet_lengths(unsigned flow_idx) const
{
    return (filter_accumulate(
        std::begin(*this),
        std::end(*this),
        0UL,
        [&](auto&& tuple) { return (std::get<0>(tuple) == flow_idx); },
        [](size_t sum, auto&& tuple) { return (sum += std::get<5>(tuple)); }));
}

size_t sequence::sum_flow_packet_lengths(unsigned flow_idx,
                                         size_t pkt_idx) const
{
    auto q = lldiv(pkt_idx, m_packet_indexes.size());

    auto sum = filter_accumulate(
        std::begin(*this),
        std::next(std::begin(*this), q.rem),
        0UL,
        [&](auto&& tuple) { return (std::get<0>(tuple) == flow_idx); },
        [](size_t sum, auto&& tuple) { return (sum += std::get<5>(tuple)); });

    if (q.quot) { sum += q.quot * sum_flow_packet_lengths(flow_idx); }

    return (sum);
}

bool sequence::has_signature_config() const
{
    const auto& signature_configs = m_definitions.get<3>();
    return (std::any_of(std::begin(signature_configs),
                        std::end(signature_configs),
                        [](const auto& item) { return (item.has_value()); }));
}

inline uint32_t to_stream_id(uint16_t stream_id, uint16_t flow_id)
{
    return (stream_id << 16 | flow_id + 1);
}

std::optional<uint32_t>
sequence::get_signature_stream_id(unsigned flow_idx) const
{
    /*
     * Find the definition index for this flow index. Use
     * our list of offsets to determine which definition contains
     * this flow.
     */
    auto cursor = std::prev(std::upper_bound(
        std::begin(m_flow_offsets), std::end(m_flow_offsets), flow_idx));
    assert(cursor != std::end(m_flow_offsets));
    auto def_idx = std::distance(std::begin(m_flow_offsets), cursor);

    const auto& signature_configs = m_definitions.get<3>();
    auto sig_config = signature_configs[def_idx];
    if (!sig_config) { return (std::nullopt); }

    assert(flow_idx >= m_flow_offsets[def_idx]);

    return (to_stream_id(sig_config->stream_id,
                         flow_idx - m_flow_offsets[def_idx]));
}

size_t sequence::flow_count() const
{
    const auto& packet_templates = m_definitions.get<0>();
    return (std::accumulate(
        std::begin(packet_templates),
        std::end(packet_templates),
        0UL,
        [](size_t lhs, const auto& rhs) { return (lhs + rhs.size()); }));
}

size_t sequence::flow_packets(unsigned flow_idx) const
{
    return (
        std::count_if(std::begin(*this), std::end(*this), [&](auto&& tuple) {
            return (std::get<0>(tuple) == flow_idx);
        }));
}

size_t sequence::size() const { return (m_size); }

namespace detail {

template <typename Function,
          typename FirstTuple,
          typename SecondTuple,
          size_t... Indexes>
constexpr void apply_two_impl(Function&& f,
                              FirstTuple&& x,
                              SecondTuple&& y,
                              std::index_sequence<Indexes...>)
{
    ((std::forward<Function>(f)(
         std::get<Indexes>(std::forward<FirstTuple>(x)),
         std::get<Indexes>(std::forward<SecondTuple>(y)))),
     ...);
}

template <typename Function, typename FirstTuple, typename SecondTuple>
constexpr void apply_two(Function&& f, FirstTuple&& x, SecondTuple&& y)
{
    apply_two_impl(
        std::forward<Function>(f),
        std::forward<FirstTuple>(x),
        std::forward<SecondTuple>(y),
        std::make_index_sequence<
            std::tuple_size_v<std::remove_reference_t<FirstTuple>>>{});
}

template <typename... OutputIt> class unzip_output_iterator
{
public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    explicit unzip_output_iterator(OutputIt... cursors)
        : m_cursors(std::make_tuple(cursors...))
    {}

    unzip_output_iterator& operator*() { return (*this); }

    template <typename... Args>
    unzip_output_iterator& operator=(const std::tuple<Args...>& tuple)
    {
        detail::apply_two(
            [](auto&& cursor, auto&& arg) { *cursor = arg; }, m_cursors, tuple);
        return (*this);
    }

    unzip_output_iterator& operator++()
    {
        std::apply([](auto&&... cursor) { (++cursor, ...); }, m_cursors);
        return (*this);
    }

    unzip_output_iterator operator++(int)
    {
        auto to_return = *this;
        std::apply([](auto&&... cursor) { (cursor++, ...); }, m_cursors);
        return (to_return);
    }

private:
    std::tuple<OutputIt...> m_cursors;
};

} // namespace detail

template <typename... OutputIt> constexpr auto unzip(OutputIt... cursors)
{
    return (detail::unzip_output_iterator<OutputIt...>(cursors...));
}

/**
 * Handy function to transform ring like data
 */
template <typename InputIt,
          typename OutputIt,
          typename Size,
          typename TransformFn>
OutputIt ring_transform_n(InputIt first,
                          InputIt last,
                          InputIt cursor,
                          Size n,
                          OutputIt d_cursor,
                          TransformFn&& xform)
{
    const auto distance = std::distance(first, last);
    if (distance == 1) {
        d_cursor = std::fill_n(d_cursor, n, xform(*cursor++));
    } else {
        auto idx = 0U;
        while (idx++ < n) {
            *d_cursor++ = xform(*cursor++);
            if (cursor == last) { cursor = first; };
        }
    }

    return (d_cursor);
}

inline std::optional<signature_config>
maybe_update_signature_config(const std::optional<signature_config>& config,
                              uint16_t flow_idx)
{
    if (!config) return (std::nullopt);

    return (
        signature_config{.stream_id = to_stream_id(config->stream_id, flow_idx),
                         .flags = config->flags,
                         .fill = config->fill});
}

uint16_t sequence::unpack(size_t start_idx,
                          uint16_t count,
                          unsigned flow_indexes[],
                          const uint8_t* headers[],
                          packetio::packet::header_lengths header_lengths[],
                          packetio::packet::packet_type::flags header_flags[],
                          std::optional<signature_config> signature_configs[],
                          uint16_t pkt_lengths[]) const
{
    auto pkt_offset = start_idx % m_packet_indexes.size();
    const auto& packet_templates = m_definitions.get<0>();
    const auto& sig_configs = m_definitions.get<3>();

    ring_transform_n(std::begin(m_packet_indexes),
                     std::end(m_packet_indexes),
                     std::begin(m_packet_indexes) + pkt_offset,
                     count,
                     unzip(flow_indexes,
                           headers,
                           header_lengths,
                           header_flags,
                           signature_configs),
                     [&](const auto& pair) {
                         return (std::make_tuple(
                             m_flow_offsets[pair.first] + pair.second,
                             packet_templates[pair.first][pair.second],
                             packet_templates[pair.first].header_lengths(),
                             packet_templates[pair.first].header_flags(),
                             maybe_update_signature_config(
                                 sig_configs[pair.first], pair.second)));
                     });

    auto len_offset = start_idx % m_length_indexes.size();
    const auto& length_templates = m_definitions.get<1>();
    ring_transform_n(std::begin(m_length_indexes),
                     std::end(m_length_indexes),
                     std::begin(m_length_indexes) + len_offset,
                     count,
                     pkt_lengths,
                     [&](const auto& pair) {
                         return (length_templates[pair.first][pair.second]);
                     });

    return (count);
}

sequence::view_type sequence::operator[](size_t idx) const
{
    const auto& packet_templates = m_definitions.get<0>();
    const auto& sig_configs = m_definitions.get<3>();
    const auto& length_templates = m_definitions.get<1>();

    auto pkt_key = m_packet_indexes[idx % m_packet_indexes.size()];
    auto len_key = m_length_indexes[idx % m_length_indexes.size()];

    return (std::make_tuple(m_flow_offsets[pkt_key.first] + pkt_key.second,
                            packet_templates[pkt_key.first][pkt_key.second],
                            packet_templates[pkt_key.first].header_lengths(),
                            packet_templates[pkt_key.first].header_flags(),
                            maybe_update_signature_config(
                                sig_configs[pkt_key.first], pkt_key.second),
                            length_templates[len_key.first][len_key.second]));
}

sequence::iterator sequence::begin() { return (iterator(*this)); }

sequence::const_iterator sequence::begin() const { return (iterator(*this)); }

sequence::iterator sequence::end() { return (iterator(*this, size())); }

sequence::const_iterator sequence::end() const
{
    return (iterator(*this, size()));
}

} // namespace openperf::packet::generator::traffic
