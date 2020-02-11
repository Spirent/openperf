#include "catch.hpp"

#include "utils/soa_container.hpp"

TEST_CASE("soa container", "[framework]")
{
    using test_container =
        openperf::utils::soa_container<std::vector, std::tuple<int, double>>;

    SECTION("can store tuples, ")
    {
        test_container container;
        container.push_back({1, 1.1});
        container.push_back({2, 2.1});
        container.push_back({3, 3.1});

        REQUIRE(container.size() == 3);

        SECTION("can iterate container as struct, ")
        {
            auto nb_items = 0;
            for (const auto& item : container) {
                REQUIRE_NOTHROW(std::get<0>(item));
                REQUIRE_NOTHROW(std::get<1>(item));
                nb_items++;
            }

            REQUIRE(nb_items == 3);
        }

        SECTION("can iterate container as array, ")
        {
            auto start0 = container.data<0>();
            REQUIRE(std::accumulate(start0, start0 + container.size(), 0) == 6);

            auto start1 = container.data<1>();
            REQUIRE(std::accumulate(start1, start1 + container.size(), 0.0)
                    == Approx(6.3));
        }
    }

    SECTION("can compare containers, ")
    {
        test_container c1;
        c1.push_back({1, 1.1});

        test_container c2;
        c2.push_back({1, 1.1});

        REQUIRE(c1 == c2);

        c2.push_back({2, 2.1});
        REQUIRE(c1 != c2);
    }
}
