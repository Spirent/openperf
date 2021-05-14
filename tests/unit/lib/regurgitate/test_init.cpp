#include "catch.hpp"
#include "test_api.hpp"

namespace test = regurgitate::test;

TEST_CASE("initialization", "[regurgitate]")
{
    auto& functions = regurgitate::impl::functions::instance();

    SECTION("functions")
    {
        /* All functions should be non-null if initialization works */
        REQUIRE(functions.merge_float_double_impl.best != nullptr);
        REQUIRE(functions.merge_float_float_impl.best != nullptr);

        REQUIRE(functions.sort_float_double_impl.best != nullptr);
        REQUIRE(functions.sort_float_float_impl.best != nullptr);
    }

    SECTION("warnings")
    {
        /*
         * Log a warning if we have a vector implementation we can't test
         * due to the host CPU. Since all function families are compiled
         * to the same targets, we only need to check one function wrapper.
         */
        unsigned fn_count = 0;
        for (auto isa : test::get_instruction_sets()) {
            auto test_fn = regurgitate::get_function(
                functions.merge_float_float_impl.functions, isa);

            if (!test_fn) {
                /* instruction set wasn't a target for this build; skip */
                continue;
            }

            fn_count++;

            if (!regurgitate::instruction_set::available(isa)) {
                WARN("Skipping "
                     << regurgitate::instruction_set::to_string(isa)
                     << "function tests due to lack of host CPU support.");
            }
        }

        REQUIRE(fn_count > 0);
    }
}
