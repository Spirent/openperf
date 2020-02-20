#include <chrono>

#include "catch.hpp"

#include "packet/analyzer/statistics/stream/counters.hpp"

using namespace openperf::packet::analyzer::statistics;

TEST_CASE("stream counters", "[packet_analyzer]")
{
    /**
     * Sequence tests come directly from Paul Mooney's Real-time Advanced
     * Sequence Checking presentation.  If the stats are wrong, blame him. ;-)
     *
     * In addition to the sequence numbers presented in the presentation,
     * we also adjust the whole sequence by an offset to verify that the correct
     * behavior is also observed when the sequence number wraps around the
     * maximum.
     */
    SECTION("sequence stats, ")
    {
        auto seq = stream::sequencing{};
        static constexpr auto late_threshold = 1000;

        SECTION("sequece one, ")
        {
            const auto test_seq = {1, 2, 3, 4, 6, 7, 5, 10, 8, 9, 11};
            const auto max =
                *std::max_element(std::begin(test_seq), std::end(test_seq));
            const auto min =
                *std::min_element(std::begin(test_seq), std::end(test_seq));

            auto verify = [&](uint32_t last_value) {
                REQUIRE(seq.last_seq.has_value());
                REQUIRE(seq.last_seq.value() == last_value);
                REQUIRE(seq.in_order == 8);
            };

            auto test = [&](uint32_t offset) {
                for (auto idx : test_seq) {
                    stream::update(seq,
                                   static_cast<uint32_t>(offset + idx),
                                   late_threshold);
                }
                verify(offset + max);
            };

            SECTION("offsets, ")
            {
                SECTION("zero, ") { test(0); }

                static constexpr auto offset =
                    std::numeric_limits<uint32_t>::max();

                SECTION("min, ") { test(offset - min); }

                SECTION("max, ") { test(offset - max); }

                SECTION("avg, ")
                {
                    const auto avg = (min + max) / 2;
                    test(offset - avg);
                }
            }
        }

        SECTION("sequence two, ")
        {
            const auto test_seq = {62, 63, 65, 66, 67, 68, 69, 70, 71, 67,
                                   72, 73, 74, 75, 70, 76, 77, 80, 81, 82};
            const auto max =
                *std::max_element(std::begin(test_seq), std::end(test_seq));
            const auto min =
                *std::min_element(std::begin(test_seq), std::end(test_seq));

            auto verify = [&](uint32_t last_value) {
                REQUIRE(seq.last_seq.has_value());
                REQUIRE(seq.last_seq.value() == last_value);
                REQUIRE(seq.run_length == 3);
                REQUIRE(seq.duplicate == 2);
                REQUIRE(seq.dropped == 3);
            };

            auto test = [&](uint32_t offset) {
                for (auto idx : test_seq) {
                    stream::update(seq,
                                   static_cast<uint32_t>(offset + idx),
                                   late_threshold);
                }
                verify(offset + max);
            };

            SECTION("offsets, ")
            {
                static constexpr auto offset =
                    std::numeric_limits<uint32_t>::max();

                SECTION("zero, ") { test(0); }

                SECTION("min, ") { test(offset - min); }

                SECTION("max, ") { test(offset - max); }

                SECTION("avg, ")
                {
                    const auto avg = (min + max) / 2;
                    test(offset - avg);
                }
            }
        }

        SECTION("sequence three, ")
        {
            const auto test_seq = {17, 18, 19, 22, 23, 24, 25, 26, 27,
                                   28, 29, 30, 31, 32, 23, 33, 34, 22,
                                   35, 20, 21, 36, 5,  37, 40};
            const auto max =
                *std::max_element(std::begin(test_seq), std::end(test_seq));
            const auto min =
                *std::min_element(std::begin(test_seq), std::end(test_seq));

            static constexpr auto test_threshold = 30;

            auto verify = [&](uint32_t last_value) {
                REQUIRE(seq.last_seq.has_value());
                REQUIRE(seq.last_seq.value() == last_value);
                REQUIRE(seq.run_length == 1);
                REQUIRE(seq.duplicate == 2);
                REQUIRE(seq.reordered == 2);
                REQUIRE(seq.late == 1);
            };

            auto test = [&](uint32_t offset) {
                for (auto idx : test_seq) {
                    stream::update(seq,
                                   static_cast<uint32_t>(offset + idx),
                                   test_threshold);
                }
                verify(offset + max);
            };

            SECTION("offsets, ")
            {
                static constexpr auto offset =
                    std::numeric_limits<uint32_t>::max();

                SECTION("zero, ") { test(0); }

                SECTION("min, ") { test(offset - min); }

                SECTION("max, ") { test(offset - max); }

                SECTION("avg, ")
                {
                    const auto avg = (min + max) / 2;
                    test(offset - avg);
                }
            }
        }

        SECTION("sequence four, ")
        {
            const auto test_seq = {1, 2, 3, 8, 9, 10, 11, 6, 12, 4, 13};
            const auto max =
                *std::max_element(std::begin(test_seq), std::end(test_seq));
            const auto min =
                *std::min_element(std::begin(test_seq), std::end(test_seq));

            auto verify = [&](uint32_t last_value) {
                REQUIRE(seq.last_seq.has_value());
                REQUIRE(seq.last_seq.value() == last_value);
                REQUIRE(seq.dropped == 2);
                REQUIRE(seq.reordered == 2);
            };

            auto test = [&](uint32_t offset) {
                for (auto idx : test_seq) {
                    stream::update(seq,
                                   static_cast<uint32_t>(offset + idx),
                                   late_threshold);
                }
                verify(offset + max);
            };

            SECTION("offsets, ")
            {
                static constexpr auto offset =
                    std::numeric_limits<uint32_t>::max();

                SECTION("zero, ") { test(0); }

                SECTION("min, ") { test(offset - min); }

                SECTION("max, ") { test(offset - max); }

                SECTION("avg, ")
                {
                    const auto avg = (min + max) / 2;
                    test(offset - avg);
                }
            }
        }
    }
}
