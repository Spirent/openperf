#include "catch.hpp"
#include "api_test.h"

TEST_CASE("spirent pga initialization", "[spirent-pga]")
{
    auto& functions = pga::functions::instance();

    SECTION("functions") {
        /* All functions should be non-null if initialization works */
        REQUIRE(functions.encode_signatures_impl.best != nullptr);
        REQUIRE(functions.decode_signatures_impl.best != nullptr);
        REQUIRE(functions.fill_step_aligned_impl.best != nullptr);
        REQUIRE(functions.fill_prbs_aligned_impl.best != nullptr);
        REQUIRE(functions.verify_prbs_aligned_impl.best != nullptr);
    }

    SECTION("warnings") {
        /*
         * Log a warning if we have vector implementations we can't test
         * due to the host CPU.  Since all functions families are compiled
         * to the same targets, we only need to check one function wrapper.
         */
        unsigned vector_fn_count = 0;
        for (auto instruction_set : pga::test::vector_instruction_sets()) {
            auto test_fn = pga::test::get_function(functions.encode_signatures_impl,
                                                   instruction_set);
            if (!test_fn) {
                /* instruction set wasn't a target for this build; skip */
                continue;
            }

            vector_fn_count++;

            /* Un-testable functions; generate a warning */
            if (!pga::instruction_set::available(instruction_set)) {
                WARN("Skipping " << pga::instruction_set::to_string(instruction_set)
                     << " function tests due to lack of host CPU support.");
            }
        }
        /*
         * No vector functions might be a problem?
         * Also, prevent generating a "no assertion" warning if catch warnings
         * are enabled.
         */
        REQUIRE(vector_fn_count > 0);
    }
}
