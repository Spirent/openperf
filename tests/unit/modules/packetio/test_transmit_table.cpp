#include "catch.hpp"

#include "core/op_uuid.hpp"
#include "packetio/transmit_table.tcc"

struct test_source
{
    std::string id_;

    std::string id() const { return (id_); }
};

template class openperf::packetio::transmit_table<test_source>;

using transmit_table = openperf::packetio::transmit_table<test_source>;

TEST_CASE("transmit table functionality", "[transmit table]")
{
    auto table = transmit_table();

    SECTION("insert one, ")
    {
        auto s1 = test_source{"source_1"};

        auto to_delete = table.insert_source(0, 0, s1);
        REQUIRE(to_delete);
        delete to_delete;

        SECTION("find one, ")
        {
            auto range = table.get_sources(0, 0);
            REQUIRE(std::distance(range.first, range.second) == 1);
            auto s1_ptr = table.get_source(range.first->first);
            REQUIRE(s1_ptr);
            REQUIRE(s1_ptr->id() == s1.id());

            SECTION("remove one, ")
            {
                to_delete = table.remove_source(0, 0, s1.id());
                REQUIRE(to_delete);
                delete to_delete;

                auto range = table.get_sources(0, 0);
                REQUIRE(std::distance(range.first, range.second) == 0);
            }
        }
    }

    SECTION("insert many, ")
    {
        std::vector<test_source> sources;
        std::vector<std::pair<uint16_t, uint16_t>> port_queue_pairs;
        static constexpr unsigned many_ports = 10;
        static constexpr unsigned many_queues = 3;

        /* Boot-strapping */
        using namespace openperf::core;
        std::generate_n(
            std::back_inserter(sources), many_ports * many_queues,
            []() { return (test_source{to_string(uuid::random())}); });

        unsigned idx = 0;
        std::generate_n(std::back_inserter(port_queue_pairs),
                        many_ports * many_queues, [&]() {
                            auto pair = std::make_pair(idx / many_queues,
                                                       idx % many_queues);
                            idx++;
                            return (pair);
                        });

        std::for_each(std::begin(port_queue_pairs), std::end(port_queue_pairs),
                      [&](const auto& pair) {
                          auto to_delete = table.insert_source(
                              pair.first, pair.second,
                              sources[pair.first * many_queues + pair.second]);
                          REQUIRE(to_delete);
                          delete to_delete;
                      });

        SECTION("find many, ")
        {
            auto ports = std::array<uint16_t, many_ports>{};
            std::iota(std::begin(ports), std::end(ports), 0);

            std::for_each(std::begin(ports), std::end(ports),
                          [&](uint16_t port_idx) {
                              auto range = table.get_sources(port_idx);
                              REQUIRE(std::distance(range.first, range.second)
                                      == many_queues);
                          });

            std::for_each(
                std::begin(port_queue_pairs), std::end(port_queue_pairs),
                [&](const auto& pair) {
                    auto range = table.get_sources(pair.first, pair.second);
                    REQUIRE(std::distance(range.first, range.second) == 1);

                    auto ptr = table.get_source(range.first->first);
                    REQUIRE(ptr);
                    REQUIRE(ptr->id()
                            == sources[pair.first * many_queues + pair.second]
                                   .id());
                });

            SECTION("remove many, ")
            {
                std::for_each(
                    std::begin(port_queue_pairs), std::end(port_queue_pairs),
                    [&](const auto& pair) {
                        auto to_delete = table.remove_source(
                            pair.first, pair.second,
                            sources[pair.first * many_queues + pair.second]
                                .id());
                        REQUIRE(to_delete);
                        delete to_delete;

                        auto range = table.get_sources(pair.first, pair.second);
                        REQUIRE(std::distance(range.first, range.second) == 0);
                    });
            }
        }
    }
}
