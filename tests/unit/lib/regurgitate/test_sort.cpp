#include <vector>

#include "catch.hpp"

#include "test_api.hpp"
#include "regurgitate/regurgitate.hpp"

namespace test = regurgitate::test;

template <typename KeyType, typename ValType, typename Container>
void test_sort(const Container& test_sizes)
{
    for (auto size : test_sizes) {
        auto means = std::vector<KeyType>();
        test::fill_random(means, size);

        auto weights = std::vector<ValType>(size);
        std::iota(std::begin(weights), std::end(weights), 1);

        auto scratch_m = std::vector<KeyType>(size, 0);
        auto scratch_w = std::vector<ValType>(size, 0);

        auto tests_executed = 0;
        auto& wrapper = test::get_sort_wrapper<KeyType, ValType>();
        for (auto isa : test::get_instruction_sets()) {
            auto sort_fn = regurgitate::get_function(wrapper.functions, isa);

            if (!(sort_fn && available(isa))) { continue; }

            INFO("instruction set = " << to_string(isa));

            tests_executed++;
            sort_fn(means.data(),
                    weights.data(),
                    size,
                    scratch_m.data(),
                    scratch_w.data());

            /* means should be sorted */
            REQUIRE(std::is_sorted(std::begin(means), std::end(means)));

            /*
             * Not sure what order weights should be in... Just sort them and
             * make sure we have all the weights we expect.
             */
            std::sort(std::begin(weights), std::end(weights));
            std::iota(std::begin(scratch_w), std::end(scratch_w), 1);
            REQUIRE(std::equal(
                std::begin(weights), std::end(weights), std::begin(scratch_w)));
        }

        REQUIRE(tests_executed > 0);
    }
}

/*
 * XXX: This test only verifies one of the implementations available for the
 * CPU, e.g. an 8-wide capable CPU won't test the 4-wide version.
 */
TEST_CASE("sorting", "[regurgitate]")
{
    /* Generate a vector of test sizes */
    auto test_sizes = std::vector<unsigned>{};
    for (auto size = 64; size < 1024; size += 63) {
        test_sizes.push_back(size);
    }

    test_sort<float, float>(test_sizes);
    test_sort<float, double>(test_sizes);
}
