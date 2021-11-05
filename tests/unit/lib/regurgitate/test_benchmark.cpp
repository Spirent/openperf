#include <random>
#include <algorithm>
#include <iterator>
#include <functional>

#include "catch.hpp"

#include "test_api.hpp"
#include "digestible/digestible.h"
#include "regurgitate/regurgitate.hpp"

namespace test = regurgitate::test;

static constexpr unsigned compression = 16;
using ref_digest = digestible::tdigest<float, float>;
using test_digest = regurgitate::digest<float, float, compression>;

template <typename Digest> void fill_random(Digest& digest, size_t count)
{
    auto device = std::random_device{};
    auto generator = std::mt19937(device());
    auto dist = std::uniform_real_distribution<>(0.0, 1000000.0);

    for (auto i = 0U; i < count; i++) { digest.insert(dist(generator)); }
}

TEST_CASE("benchmark", "[regurgitate]")
{
    /*
     * Using a multiple of the optimum length ensures that all data
     * points will be merged.
     */
    constexpr size_t size = regurgitate_optimum_length * 1000;

    BENCHMARK("digestible<float, float>")
    {
        auto digest = digestible::tdigest<float, float>(compression);
        fill_random(digest, size);
        digest.merge();
    };

    for (auto isa : test::get_instruction_sets()) {
        auto& merge_wrapper = test::get_merge_wrapper<float, float>();

        auto merge_fn = regurgitate::get_function(merge_wrapper.functions, isa);
        if (!(merge_fn && available(isa))) { continue; }

        /*
         * We use a dirty, dirty hack to benchmark individual ISA
         * implementations... We change the conditions of the test.
         */
        merge_wrapper.best = merge_fn;

        auto& sort_wrapper = test::get_sort_wrapper<float, float>();
        auto sort_fn = regurgitate::get_function(sort_wrapper.functions, isa);
        sort_wrapper.best = sort_fn;

        BENCHMARK("regurgitate<float, float> " + std::string(to_string(isa)))
        {
            auto digest = regurgitate::digest<float, float, compression>();
            fill_random(digest, size);
        };
    }

    BENCHMARK("digestible<float, double>")
    {
        auto digest = digestible::tdigest<float, double>(compression);
        fill_random(digest, size);
        digest.merge();
    };

    for (auto isa : test::get_instruction_sets()) {
        auto& merge_wrapper = test::get_merge_wrapper<float, double>();

        auto merge_fn = regurgitate::get_function(merge_wrapper.functions, isa);
        if (!(merge_fn && available(isa))) { continue; }

        merge_wrapper.best = merge_fn;

        auto& sort_wrapper = test::get_sort_wrapper<float, double>();
        auto sort_fn = regurgitate::get_function(sort_wrapper.functions, isa);
        sort_wrapper.best = sort_fn;

        BENCHMARK("regurgitate<float, double> " + std::string(to_string(isa)))
        {
            auto digest = regurgitate::digest<float, double, compression>();
            fill_random(digest, size);
        };
    }
}
