#include "catch.hpp"
#include "api_test.h"

TEST_CASE("spirent pga initialization", "[spirent-pga]")
{
    auto& functions = pga::functions::instance();

    /* All functions should be non-null if initialization works */
    REQUIRE(functions.encode_signatures_impl.best != nullptr);
    REQUIRE(functions.decode_signatures_impl.best != nullptr);
    REQUIRE(functions.fill_step_aligned_impl.best != nullptr);
    REQUIRE(functions.fill_prbs_aligned_impl.best != nullptr);
    REQUIRE(functions.verify_prbs_aligned_impl.best != nullptr);
}
