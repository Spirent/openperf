#include <map>
#include <utility>

#include "catch.hpp"

#include "core/op_uuid.hpp"
#include "packetio/forwarding_table.tcc"

static constexpr int max_ports = 32;

struct test_interface
{
    std::string id;
};

struct test_sink
{
    std::string id;

    bool operator==(const test_sink& other) const { return (id == other.id); }
};

template class openperf::packetio::
    forwarding_table<test_interface, test_sink, max_ports>;

template <>
std::string openperf::packetio::get_interface_id(test_interface* ifp)
{
    return (ifp->id);
}

using forwarding_table =
    openperf::packetio::forwarding_table<test_interface, test_sink, max_ports>;

TEST_CASE("forwarding table functionality", "[forwarding table]")
{
    auto table = forwarding_table();

    SECTION("insert interface, ")
    {
        auto ifp1 = test_interface{"interface_1"};
        auto mac1 =
            openperf::net::mac_address{0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
        auto port1 = static_cast<uint16_t>(0);

        auto to_delete =
            table.insert_interface(port1, mac1, std::addressof(ifp1));
        REQUIRE(to_delete);
        delete to_delete;

        SECTION("find interface, ")
        {
            auto ptr = table.find_interface(port1, mac1);
            REQUIRE(ptr == std::addressof(ifp1));
            REQUIRE(ptr->id == ifp1.id);

            auto ptr2 = table.find_interface(ifp1.id);
            REQUIRE(ptr2 == std::addressof(ifp1));
            REQUIRE(ptr2->id == ifp1.id);

            SECTION("remove interface, ")
            {
                to_delete = table.remove_interface(port1, mac1);
                REQUIRE(to_delete);
                delete to_delete;

                auto ptr = table.find_interface(port1, mac1);
                REQUIRE(!ptr);
            }
        }
    }

    SECTION("insert sink, ")
    {
        auto sink1 = test_sink{"sink_1"};
        auto port1 = static_cast<uint16_t>(3);

        auto to_delete = table.insert_sink(port1, sink1);
        REQUIRE(to_delete);
        delete to_delete;

        SECTION("find sink, ")
        {
            auto sink_vec = table.get_sinks(port1);
            REQUIRE(sink_vec.size() == 1);
            REQUIRE(sink_vec[0].id == sink1.id);

            auto sink_vec2 = table.get_sinks(port1 + 1);
            REQUIRE(sink_vec2.empty());

            SECTION("remove interface, ")
            {
                to_delete = table.remove_sink(port1, sink1);
                REQUIRE(to_delete);
                delete to_delete;

                auto sink_vec3 = table.get_sinks(port1);
                REQUIRE(sink_vec3.empty());
            }
        }
    }

    SECTION("insert many interfaces, ")
    {
        static constexpr unsigned many_interfaces = 20;
        static constexpr unsigned many_ports = 10;

        /* make sure we get the same number of interfaces on each port */
        static_assert(many_interfaces % many_ports == 0);

        /* Create a map of port, mac --> interfaces and load them into our table
         */
        using vif_map =
            std::map<std::pair<uint16_t, openperf::net::mac_address>,
                     test_interface>;
        auto interfaces = vif_map();

        auto if_idx = 0;
        std::generate_n(
            std::inserter(interfaces, interfaces.begin()),
            many_interfaces,
            [&]() {
                auto uuid = openperf::core::uuid::random();
                auto mac = openperf::net::mac_address(uuid.data());
                auto key = std::make_pair(if_idx % many_ports, mac);
                if_idx++;
                return (std::make_pair(
                    key, test_interface{openperf::core::to_string(uuid)}));
            });

        for (auto& [key, value] : interfaces) {
            table.insert_interface(
                key.first, key.second, std::addressof(value));
        }

        SECTION("find many interfaces, ")
        {
            for (const auto& [key, value] : interfaces) {
                auto ptr = table.find_interface(key.first, key.second);
                REQUIRE(ptr->id == value.id);

                auto ptr2 = table.find_interface(value.id);
                REQUIRE(ptr2->id == value.id);

                auto& map = table.get_interfaces(key.first);
                REQUIRE(map.size() == many_interfaces / many_ports);
            }

            SECTION("remove many interfaces, ")
            {
                for (const auto& [key, value] : interfaces) {
                    auto to_delete =
                        table.remove_interface(key.first, key.second);
                    REQUIRE(to_delete);
                    delete to_delete;
                }

                for (const auto& [key, value] : interfaces) {
                    auto ptr = table.find_interface(key.first, key.second);
                    REQUIRE(!ptr);
                }
            }
        }
    }

    SECTION("insert many sinks, ")
    {
        static constexpr unsigned many_sinks = 8;
        static constexpr uint16_t port0 = 0;
        static constexpr uint16_t port1 = 1;

        std::vector<test_sink> port0_sinks;
        std::vector<test_sink> port1_sinks;

        std::generate_n(std::back_inserter(port0_sinks), many_sinks, [&]() {
            return (test_sink{
                openperf::core::to_string(openperf::core::uuid::random())});
        });
        std::generate_n(std::back_inserter(port1_sinks), many_sinks, [&]() {
            return (test_sink{
                openperf::core::to_string(openperf::core::uuid::random())});
        });

        for (const auto& sink : port0_sinks) {
            auto to_delete = table.insert_sink(port0, sink);
            REQUIRE(to_delete);
            delete to_delete;
        }

        for (const auto& sink : port1_sinks) {
            auto to_delete = table.insert_sink(port1, sink);
            REQUIRE(to_delete);
            delete to_delete;
        }

        SECTION("find many sinks, ")
        {
            auto table_port0_sinks = table.get_sinks(port0);
            REQUIRE(table_port0_sinks.size() == many_sinks);
            std::vector<test_sink> difference;
            std::set_difference(std::begin(port0_sinks),
                                std::end(port0_sinks),
                                std::begin(table_port0_sinks),
                                std::end(table_port0_sinks),
                                std::back_inserter(difference),
                                [](const test_sink& a, const test_sink& b) {
                                    return (a.id < b.id);
                                });
            REQUIRE(difference.empty());

            auto table_port1_sinks = table.get_sinks(port1);
            REQUIRE(table_port1_sinks.size() == many_sinks);

            std::set_difference(std::begin(port1_sinks),
                                std::end(port1_sinks),
                                std::begin(table_port1_sinks),
                                std::end(table_port1_sinks),
                                std::back_inserter(difference),
                                [](const test_sink& a, const test_sink& b) {
                                    return (a.id < b.id);
                                });
            REQUIRE(difference.empty());

            SECTION("remove many sinks, ")
            {
                REQUIRE(table.get_sinks(port0).size() == many_sinks);
                REQUIRE(table.get_sinks(port1).size() == many_sinks);

                for (const auto& sink : port0_sinks) {
                    auto to_delete = table.remove_sink(port0, sink);
                    REQUIRE(to_delete);
                    delete to_delete;
                }

                REQUIRE(table.get_sinks(port0).empty());

                for (const auto& sink : port1_sinks) {
                    auto to_delete = table.remove_sink(port1, sink);
                    REQUIRE(to_delete);
                    delete to_delete;
                }

                REQUIRE(table.get_sinks(port1).empty());
            }
        }
    }
}
