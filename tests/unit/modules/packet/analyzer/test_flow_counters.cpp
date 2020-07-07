#include <chrono>

#include "catch.hpp"

#include "packet/analyzer/statistics/flow/counters.hpp"

using namespace openperf::packet::analyzer::statistics;

TEST_CASE("flow counters", "[packet_analyzer]")
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
        auto seq = flow::counter::sequencing{};
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
                    flow::counter::update(seq,
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
                    flow::counter::update(seq,
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
                    flow::counter::update(seq,
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
                    flow::counter::update(seq,
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

    SECTION("summary stats, ")
    {
        SECTION("variance, ")
        {
            const auto test_data = {3, 5, 7, 7, 38};
            /*
             * Expected variance for our test data is 214.  However, the
             * variance is stored as a sum and needs to be divided by (n - 1).
             */
            const auto m2_exp = Approx(214 * (test_data.size() - 1));

            SECTION("double values, ")
            {
                using test_summary_type =
                    flow::counter::summary<double, double>;

                SECTION("verify stats, ")
                {
                    auto test_summary = test_summary_type{};
                    auto idx = 0;
                    for (auto& item : test_data) {
                        update(test_summary, item, ++idx);
                    }

                    REQUIRE(test_summary.min == 3);
                    REQUIRE(test_summary.max == 38);
                    REQUIRE(test_summary.total
                            == std::accumulate(
                                std::begin(test_data), std::end(test_data), 0));

                    REQUIRE(test_summary.m2 == m2_exp);
                }

                SECTION("verify weighted addition, ")
                {
                    auto sum1 = test_summary_type{};
                    auto sum2 = test_summary_type{};

                    update(sum1, 3, 1);
                    update(sum1, 5, 2);
                    update(sum1, 7, 3);

                    update(sum2, 7, 1);
                    update(sum2, 38, 2);

                    auto m2 = flow::counter::add_variance(
                        3, sum1.total, sum1.m2, 2, sum2.total, sum2.m2);
                    REQUIRE(m2 == m2_exp);
                }
            }

            SECTION("integer values, ")
            {
                using test_summary_type = flow::counter::summary<int, int>;
                auto test_summary = test_summary_type{};

                auto idx = 0;
                for (auto& item : test_data) {
                    update(test_summary, item, ++idx);
                }

                SECTION("verify stats, ")
                {
                    REQUIRE(test_summary.min == 3);
                    REQUIRE(test_summary.max == 38);
                    REQUIRE(test_summary.total
                            == std::accumulate(
                                std::begin(test_data), std::end(test_data), 0));

                    REQUIRE(test_summary.m2 == m2_exp);
                }
            }

            SECTION("duration values, ")
            {
                using test_summary_type =
                    flow::counter::summary<std::chrono::nanoseconds,
                                           std::chrono::nanoseconds>;
                SECTION("verify stats, ")
                {
                    auto test_summary = test_summary_type{};

                    auto idx = 0;
                    for (auto& item : test_data) {
                        update(test_summary,
                               std::chrono::nanoseconds{item},
                               ++idx);
                    }

                    REQUIRE(test_summary.min.count() == 3);
                    REQUIRE(test_summary.max.count() == 38);
                    REQUIRE(test_summary.total.count()
                            == std::accumulate(
                                std::begin(test_data), std::end(test_data), 0));

                    REQUIRE(test_summary.m2.count() == m2_exp);
                }

                SECTION("verify weighted addition, ")
                {
                    auto sum1 = test_summary_type{};
                    auto sum2 = test_summary_type{};

                    using namespace std::chrono_literals;
                    update(sum1, 3ns, 1);
                    update(sum1, 5ns, 2);
                    update(sum1, 7ns, 3);

                    update(sum2, 7ns, 1);
                    update(sum2, 38ns, 2);

                    auto m2 = flow::counter::add_variance(
                        3, sum1.total, sum1.m2, 2, sum2.total, sum2.m2);
                    REQUIRE(m2.count() == m2_exp);
                }
            }
        }
    }
}
