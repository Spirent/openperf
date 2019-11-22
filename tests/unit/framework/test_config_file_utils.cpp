#include "catch.hpp"
#include <string>
#include <string.h>

#include "op_config_file_utils.hpp"
#include "yaml_json_emitter.hpp"

using namespace openperf::config::file;

TEST_CASE("check config file utilty functions", "[config file]")
{
    SECTION("verify is_number() function")
    {
        // clang-format off
        auto is_number_true = GENERATE(as<std::string> {},
                                       "20",
                                       "12839831",
                                       "1.5",
                                       "0.00005",
                                       "50",
                                       "1.593",
                                       "0x10",
                                       "0x1abcd");
        // clang-format on
        REQUIRE(op_config_is_number(is_number_true) == true);

        // clang-format off
        auto is_number_false = GENERATE(as<std::string> {},
                                        "nope",
                                        "O050",
                                        "1.1.1.1",
                                        "ab:cd:ef:gh",
                                        "2001::4",
                                        "12nope",
                                        "14 42",
                                        "50-30");
        // clang-format on
        REQUIRE(op_config_is_number(is_number_false) == false);
    }

    SECTION("verify splitting path and id function")
    {
        auto path_id = op_config_split_path_id("/ports/transmit-port-one");
        REQUIRE(std::get<0>(path_id) == "/ports");
        REQUIRE(std::get<1>(path_id) == "transmit-port-one");

        path_id = op_config_split_path_id("/ports");
        REQUIRE(std::get<0>(path_id) == "/ports");
        REQUIRE(std::get<1>(path_id) == "");

        path_id = op_config_split_path_id("");
        REQUIRE(std::get<0>(path_id) == "");
        REQUIRE(std::get<1>(path_id) == "");

        // Relative path. Function is doing its job even though
        // it might not be valid for pistache routing.
        path_id = op_config_split_path_id("ports/transmit-port-one");
        REQUIRE(std::get<0>(path_id) == "ports");
        REQUIRE(std::get<1>(path_id) == "transmit-port-one");

        path_id = op_config_split_path_id("/interfaces//interface-one");
        REQUIRE(std::get<0>(path_id) == "/interfaces/");
        REQUIRE(std::get<1>(path_id) == "interface-one");

        // Technically an error for pistache routing, but this function
        // need not know that.
        path_id = op_config_split_path_id("/interfaces//interface-one/");
        REQUIRE(std::get<0>(path_id) == "/interfaces//interface-one");
        REQUIRE(std::get<1>(path_id) == "");

        // Will probably never have this path, but worth checking
        // that the split function works here too.
        path_id = op_config_split_path_id("/abcdefg/h-i-j-k");
        REQUIRE(std::get<0>(path_id) == "/abcdefg");
        REQUIRE(std::get<1>(path_id) == "h-i-j-k");
    }
}

TEST_CASE("test YAML -> JSON emitter."
          "[config file]")
{
    // Boilerplate objects. They are not used in OpenPerf,
    // but are required by yaml-cpp interface.
    YAML::Mark mark;
    YAML::anchor_t anchor = 0;
    YAML::EmitterStyle::value style = YAML::EmitterStyle::Default;

    SECTION("basic emitter test")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);

        // Test trying to access empty state stack.
        emitter.OnScalar(mark, "", anchor, "hello");

        // Try a simple YAML document:
        // hello: there
        emitter.OnMapStart(mark, "", anchor, style);
        REQUIRE(output_stream.str() == "");

        emitter.OnScalar(mark, "", anchor, "hello");
        REQUIRE(output_stream.str() == "{\"hello\"");

        emitter.OnScalar(mark, "", anchor, "there");
        REQUIRE(output_stream.str() == "{\"hello\": \"there\"");

        emitter.OnMapEnd();
        REQUIRE(output_stream.str() == "{\"hello\": \"there\"}");
    }

    SECTION("test emitter sequence")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);
        std::string expected_output;

        // Simple YAML sequence (array).
        // Also test mixing input types (string, int, float)
        // [1, two, 3, four, 5.0]
        emitter.OnSequenceStart(mark, "", anchor, style);
        REQUIRE(output_stream.str() == "");

        emitter.OnScalar(mark, "?", anchor, "1");
        expected_output += "[1";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "two");
        expected_output += ", \"two\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "3");
        expected_output += ", 3";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "5.0");
        expected_output += ", 5.0";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);
    }

    SECTION("test emitter scalar conversion")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);
        std::string expected_output;

        // Test different input types mapping to the correct output.
        // Also tests tag parameter.
        // Tag ! -> quoted string read from input.
        // Tag ? -> unquoted string of characters read from input.
        // Use a simple sequence (array).
        // Input: [1, "1", "two", two, true, false, "true", "false", 6.7, "6.7"]
        emitter.OnSequenceStart(mark, "", anchor, style);
        REQUIRE(output_stream.str() == "");

        emitter.OnScalar(mark, "?", anchor, "1");
        expected_output += "[1";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "!", anchor, "1");
        expected_output += ", \"1\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "!", anchor, "two");
        expected_output += ", \"two\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "two");
        expected_output += ", \"two\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "true");
        expected_output += ", true";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "false");
        expected_output += ", false";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "!", anchor, "true");
        expected_output += ", \"true\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "!", anchor, "false");
        expected_output += ", \"false\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "6.7");
        expected_output += ", 6.7";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "!", anchor, "6.7");
        expected_output += ", \"6.7\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);
    }

    SECTION("test emitter maps in a sequence")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);
        std::string expected_output;

        // Make sure we can encapsulate maps within a sequence.
        // Input: [{a: b, c: d}, {one: two, three: four}]
        emitter.OnSequenceStart(mark, "", anchor, style);
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapStart(mark, "", anchor, style);
        expected_output += "[";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "a");
        expected_output += "{\"a\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "b");
        expected_output += ": \"b\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "c");
        expected_output += ", \"c\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "d");
        expected_output += ": \"d\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapEnd();
        expected_output += "}";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapStart(mark, "", anchor, style);
        expected_output += ", ";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "one");
        expected_output += "{\"one\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "two");
        expected_output += ": \"two\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "three");
        expected_output += ", \"three\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "four");
        expected_output += ": \"four\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapEnd();
        expected_output += "}";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);
    }

    SECTION("test sequences inside a map")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);
        std::string expected_output;

        // Make sure we can encapsulate sequences within a map.
        // Input: {one: [a, b], ten: [z, x]}
        emitter.OnMapStart(mark, "", anchor, style);
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "one");
        expected_output += "{\"one\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceStart(mark, "", anchor, style);
        expected_output += ": ";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "a");
        expected_output += "[\"a\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "b");
        expected_output += ", \"b\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "ten");
        expected_output += ", \"ten\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceStart(mark, "", anchor, style);
        expected_output += ": ";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "z");
        expected_output += "[\"z\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "x");
        expected_output += ", \"x\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapEnd();
        expected_output += "}";
        REQUIRE(output_stream.str() == expected_output);
    }

    SECTION("test maps in a map")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);
        std::string expected_output;

        // Make sure we can encapsulate sequences within a sequence.
        // Input: {one: {a: b, c: d}, ten: {w: y, x: z}}
        emitter.OnMapStart(mark, "", anchor, style);
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "one");
        expected_output += "{\"one\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapStart(mark, "", anchor, style);
        expected_output += ": ";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "a");
        expected_output += "{\"a\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "b");
        expected_output += ": \"b\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "c");
        expected_output += ", \"c\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "d");
        expected_output += ": \"d\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapEnd();
        expected_output += "}";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "ten");
        expected_output += ", \"ten\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapStart(mark, "", anchor, style);
        expected_output += ": ";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "w");
        expected_output += "{\"w\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "y");
        expected_output += ": \"y\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "x");
        expected_output += ", \"x\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "z");
        expected_output += ": \"z\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapEnd();
        expected_output += "}";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnMapEnd();
        expected_output += "}";
        REQUIRE(output_stream.str() == expected_output);
    }

    SECTION("test sequences in a sequence")
    {
        std::ostringstream output_stream;
        yaml_json_emitter emitter(output_stream);
        std::string expected_output;

        // Make sure we can encapsulate sequences within a sequence.
        // Input: [[a, c], [w, z]]
        emitter.OnSequenceStart(mark, "", anchor, style);
        expected_output += "";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceStart(mark, "", anchor, style);
        expected_output += "[";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "a");
        expected_output += "[\"a\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "c");
        expected_output += ", \"c\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceStart(mark, "", anchor, style);
        expected_output += ", ";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "w");
        expected_output += "[\"w\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnScalar(mark, "?", anchor, "z");
        expected_output += ", \"z\"";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);

        emitter.OnSequenceEnd();
        expected_output += "]";
        REQUIRE(output_stream.str() == expected_output);
    }
}
