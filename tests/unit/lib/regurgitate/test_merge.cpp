#include <algorithm>
#include <vector>

#include "catch.hpp"

#include "test_api.hpp"
#include "regurgitate/regurgitate.hpp"

namespace test = regurgitate::test;

static constexpr unsigned compression = 16;
using test_digest = regurgitate::digest<float, float, compression>;

template <typename Digest> void verify_digest(const Digest& digest)
{
    auto centroids = digest.get();

    REQUIRE(std::is_sorted(std::begin(centroids),
                           std::end(centroids),
                           [](const auto& lhs, const auto& rhs) {
                               return (lhs.first < rhs.first);
                           }));
}

template <typename InputIt> void dump(InputIt cursor, InputIt end)
{
    std::cout << '[';
    while (cursor != end) {
        fprintf(stdout, "%.12f, ", *cursor);
        cursor++;
    }
    std::cout << "\b\b]" << std::endl;
}

template <typename Container> void verify_test_data(const Container& c)
{
    auto digest = test_digest();
    std::for_each(std::begin(c), std::end(c), [&](const auto& pair) {
        digest.insert(pair.first, pair.second);
    });
    digest.merge();
    verify_digest(digest);
}

template <typename KeyType,
          typename ValType,
          size_t Compression,
          typename Container>
void test_merge(const Container& input_sizes)
{
    for (auto size : input_sizes) {
        auto input_means = std::vector<KeyType>();
        test::fill_random(input_means, size);
        /* input must be sorted for merging */
        std::sort(std::begin(input_means), std::end(input_means));

        auto input_weights = std::vector<ValType>(size);
        std::iota(std::begin(input_weights), std::end(input_weights), 1);

        auto tests_executed = 0;
        auto& wrapper = test::get_merge_wrapper<KeyType, ValType>();
        for (auto isa : test::get_instruction_sets()) {
            auto merge_fn = regurgitate::get_function(wrapper.functions, isa);

            if (!(merge_fn && available(isa))) { continue; }

            INFO("instruction set = " << to_string(isa));

            tests_executed++;

            auto output_means = std::array<KeyType, Compression>{};
            auto output_weights = std::array<ValType, Compression>{};

            auto end = merge_fn(input_means.data(),
                                input_weights.data(),
                                size,
                                output_means.data(),
                                output_weights.data(),
                                Compression);

            /*
             * Output should still be sorted... mostly
             * Rounding errors can affect output, so round up when checking.
             */
            auto* means = output_means.data();
            REQUIRE(std::is_sorted(
                means, means + end, [](const auto& lhs, const auto& rhs) {
                    return (ceil(lhs) < ceil(rhs));
                }));

            /* Input and Output weights should be equal */
            auto* weights = output_weights.data();
            auto total_in_weight = std::accumulate(
                std::begin(input_weights), std::end(input_weights), 0);
            auto total_out_weight = std::accumulate(weights, weights + end, 0);
            REQUIRE(total_in_weight == total_out_weight);
        }
    }
}

TEST_CASE("merge", "[regurgitate]")
{
    auto test_sizes = std::vector<unsigned>{64, 128, 256, 512};

    test_merge<float, double, 8>(test_sizes);
    test_merge<float, double, 12>(test_sizes);
    test_merge<float, double, 16>(test_sizes);

    test_merge<float, float, 8>(test_sizes);
    test_merge<float, float, 12>(test_sizes);
    test_merge<float, float, 16>(test_sizes);
}
