#include "catch.hpp"
#include <string>
#include <iostream>

#include "op_config_utils.hpp"

using namespace openperf::config;

TEST_CASE("check configuration utility functions", "[config]")
{
    SECTION("verify resource ID validation function")
    {
        // GENERATE does not play nicely with REQUIRE when the return type is tl::expected.
        // It works but does not tell you which value caused a failure.

        // clang-format off
        std::vector<std::string> valid_ids {
            "interface",
            "iface",
            "port0x2",
            "stream-34",
            "bgp-route-block",
            "0x2"};
        // clang-format on
        for (auto is_valid_id : valid_ids) {
            REQUIRE(op_config_validate_id_string(is_valid_id).error() == "");
        }

        // clang-format off
        std::vector<std::string> invalid_ids {
            "Interface",
            "iface.2",
            "port_0x2",
            "stream/34",
            "BGP-route-block",
            "0x2\5",
            "port@2",
            ""};
        // clang-format on
        for (auto not_valid_id : invalid_ids) {
            REQUIRE(op_config_validate_id_string(not_valid_id).error() != "");
        }
    }
}
