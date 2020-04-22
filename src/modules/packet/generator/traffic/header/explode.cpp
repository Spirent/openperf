#include "packet/generator/traffic/header/explode.hpp"

namespace openperf::packet::generator::traffic::header {

static container explode_zip(const std::vector<container>& headers)
{
    /* Length is the LCM of header counts in each container */
    auto seq_length = std::accumulate(std::begin(headers),
                                      std::end(headers),
                                      1UL,
                                      [](size_t lhs, const auto& rhs) {
                                          return (std::lcm(lhs, rhs.size()));
                                      });

    auto combined_length = std::accumulate(
        std::begin(headers),
        std::end(headers),
        0UL,
        [](size_t lhs, const auto& rhs) { return (lhs + rhs[0].second); });

    auto flattened = container{};
    auto buffer = std::vector<uint8_t>{};
    buffer.reserve(combined_length);

    for (auto idx = 0UL; idx < seq_length; ++idx) {
        buffer.clear();
        auto cursor = buffer.data();
        std::for_each(
            std::begin(headers), std::end(headers), [&](auto&& container) {
                auto&& item = container[idx % container.size()];
                cursor = std::copy_n(item.first, item.second, cursor);
            });

        flattened.emplace_back(buffer.data(),
                               std::distance(buffer.data(), cursor));
    }

    return (flattened);
}

template <typename InputIt, typename OutputIt, typename UnaryOperation>
constexpr OutputIt partial_product(InputIt cursor,
                                   InputIt last,
                                   OutputIt d_cursor,
                                   UnaryOperation&& op)
{
    if (cursor == last) return (d_cursor);

    using product_type = typename std::result_of<UnaryOperation(
        typename InputIt::value_type)>::type;
    auto product = product_type{1};
    *d_cursor = product;

    /* Generate products from 1 to n - 1 */
    do {
        product *= op(*cursor);
        *++d_cursor = product;
    } while (++cursor != std::prev(last));

    return (++d_cursor);
}

static container explode_cartesian(const std::vector<container>& headers)
{
    /* Length is the product of header counts in each container */
    auto seq_length = std::accumulate(
        std::begin(headers),
        std::end(headers),
        1UL,
        [](size_t lhs, const auto& rhs) { return (lhs * rhs.size()); });

    auto combined_length = std::accumulate(
        std::begin(headers),
        std::end(headers),
        0UL,
        [](size_t lhs, const auto& rhs) { return (lhs + rhs[0].second); });

    /*
     * We treat our vector of containers like a multi-digit number. Each
     * container in the vector has a range of values.  This basis vector
     * contains the scalar to apply to each "digit".
     *
     * Consider a vector containing three header containers, with 2, 3, and 4
     * different headers respectively.  The basis vector would contain
     * {12, 4, 1}.
     *
     * Hence, dividing the overall index with the associated basis value gives
     * us the index of the header for the current top level index, modulo the
     * size of the container.
     *
     * Note: we have to generate the basis via a reverse iterator, since
     * we increment the upper layer headers first.
     */
    auto basis = std::vector<size_t>{};
    partial_product(std::make_reverse_iterator(std::end(headers)),
                    std::make_reverse_iterator(std::begin(headers)),
                    std::back_inserter(basis),
                    [](const auto& container) { return (container.size()); });
    std::reverse(std::begin(basis), std::end(basis));

    auto nb_containers = headers.size();

    auto flattened = container{};
    auto buffer = std::vector<uint8_t>{};
    buffer.reserve(combined_length);

    for (auto i = 0UL; i < seq_length; ++i) {
        /* Write the proper headers into the buffer */
        buffer.clear();
        auto cursor = buffer.data();
        for (auto j = 0UL; j < nb_containers; ++j) {
            auto idx = (i / basis[j]) % headers[j].size();
            auto&& item = headers[j][idx];
            cursor = std::copy_n(item.first, item.second, cursor);
        }
        /* Add the full header to our output container */
        flattened.emplace_back(buffer.data(),
                               std::distance(buffer.data(), cursor));
    }

    return (flattened);
}

container explode(const std::vector<container>& headers, modifier_mux mux)

{
    assert(mux != modifier_mux::none);
    return (mux == modifier_mux::zip ? explode_zip(headers)
                                     : explode_cartesian(headers));
};

} // namespace openperf::packet::generator::traffic::header
