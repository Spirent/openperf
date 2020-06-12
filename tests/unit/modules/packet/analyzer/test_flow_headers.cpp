#include "catch.hpp"

#include "packet/analyzer/statistics/flow/header_view.hpp"
#include "packet/protocol/protocols.hpp"

using namespace openperf::packetio::packet::packet_type;

constexpr uint16_t test_packet_length = 512;
constexpr uint16_t truncated_packet_length = 40;

constexpr auto test_packet_one_type =
    ethernet::ether | ip::ipv4 | protocol::udp;
constexpr auto test_packet_one_header_length = 14 + 20 + 8;

struct test_packet_one
{
    libpacket::protocol::ethernet eth;
    libpacket::protocol::ipv4 ip;
    libpacket::protocol::udp udp;
    std::array<uint8_t, test_packet_length - test_packet_one_header_length>
        payload;
};

static test_packet_one get_test_packet_one()
{
    auto p = test_packet_one{};
    return (p);
}

constexpr auto test_packet_two_type = ethernet::mpls | ip::ipv6 | protocol::tcp;
constexpr auto test_packet_two_header_length = 14 + 4 + 4 + 40 + 20;

struct test_packet_two
{
    libpacket::protocol::ethernet eth;
    libpacket::protocol::mpls mpls1;
    libpacket::protocol::mpls mpls2;
    libpacket::protocol::ipv6 ipv6;
    libpacket::protocol::tcp tcp;
    std::array<uint8_t, test_packet_length - test_packet_two_header_length>
        payload;
};

static test_packet_two get_test_packet_two(bool bottom_of_stack)
{
    auto p = test_packet_two{};
    set_mpls_bottom_of_stack(p.mpls2, bottom_of_stack);
    return (p);
}

template <typename Thing> const uint8_t* to_pointer(const Thing& t)
{
    return (reinterpret_cast<const uint8_t*>(std::addressof(t)));
}

TEST_CASE("flow headers", "[packet_analyzer]")
{
    using namespace openperf::packet::analyzer::statistics;

    SECTION("lengths")
    {
        auto tp1 = get_test_packet_one();
        REQUIRE(flow::header_length(
                    test_packet_one_type, to_pointer(tp1), test_packet_length)
                == test_packet_one_header_length);
        REQUIRE(flow::header_length(test_packet_one_type,
                                    to_pointer(tp1),
                                    truncated_packet_length)
                == truncated_packet_length);

        auto tp2 = get_test_packet_two(true);
        REQUIRE(flow::header_length(
                    test_packet_two_type, to_pointer(tp2), test_packet_length)
                == test_packet_two_header_length);
        REQUIRE(flow::header_length(test_packet_two_type,
                                    to_pointer(tp2),
                                    truncated_packet_length)
                == truncated_packet_length);
    }

    SECTION("views")
    {
        auto tuple = std::tuple<flow::header>{};
        auto tp1 = get_test_packet_one();
        set_header(tuple, test_packet_one_type, to_pointer(tp1));

        auto view1 = flow::header_view(std::get<flow::header>(tuple));
        REQUIRE(view1.size() == 3);
        REQUIRE_NOTHROW(
            std::get<const libpacket::protocol::ethernet*>(view1[0]));
        REQUIRE_NOTHROW(std::get<const libpacket::protocol::ipv4*>(view1[1]));
        REQUIRE_NOTHROW(std::get<const libpacket::protocol::udp*>(view1[2]));

        auto count1 = 0;
        std::for_each(
            std::begin(view1), std::end(view1), [&](const auto&) { count1++; });
        REQUIRE(view1.size() == count1);

        /* We need to know where the header cutoff is to validate the decode */
        static_assert(flow::max_header_length == 124, "Update me!");
        auto tp2 = get_test_packet_two(true);
        set_header(tuple, test_packet_two_type, to_pointer(tp2));

        auto view2 = flow::header_view(std::get<flow::header>(tuple));
        REQUIRE(view2.size() == 5);
        REQUIRE_NOTHROW(
            std::get<const libpacket::protocol::ethernet*>(view2[0]));
        REQUIRE_NOTHROW(std::get<const libpacket::protocol::mpls*>(view2[1]));
        REQUIRE_NOTHROW(std::get<const libpacket::protocol::mpls*>(view2[2]));
        REQUIRE_NOTHROW(std::get<const libpacket::protocol::ipv6*>(view2[3]));
        REQUIRE_NOTHROW(std::get<const libpacket::protocol::tcp*>(view2[4]));

        auto count2 = 0;
        std::for_each(
            std::begin(view2), std::end(view2), [&](const auto&) { count2++; });
        REQUIRE(view2.size() == count2);
    }

    SECTION("bad mpls parsing")
    {
        static_assert(flow::max_header_length < test_packet_length,
                      "Update me");
        auto tp = get_test_packet_two(false);
        REQUIRE(flow::header_length(test_packet_two_type,
                                    to_pointer(tp),
                                    flow::max_header_length)
                == flow::max_header_length);

        auto tuple = std::tuple<flow::header>{};
        set_header(tuple, test_packet_two_type, to_pointer(tp));
        auto view = flow::header_view(std::get<flow::header>(tuple));

        /*
         * Since the bottom of stack isn't set; the parser doesn't
         * know when to stop.  However, it should find at least three
         * protocols, as the first three are correct.
         */
        REQUIRE(view.size() >= 3);

        REQUIRE_NOTHROW(
            std::get<const libpacket::protocol::ethernet*>(view[0]));

        /* It's MPLS all the way down... */
        for (size_t i = 1; i < view.size() - 1; i++) {
            REQUIRE_NOTHROW(
                std::get<const libpacket::protocol::mpls*>(view[i]));
        }

        /* Except the last header gets truncated */
        REQUIRE_NOTHROW(
            std::get<std::basic_string_view<uint8_t>>(view[view.size() - 1]));
    }
}
