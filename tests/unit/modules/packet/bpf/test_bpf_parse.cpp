#include <map>
#include <utility>

#include "catch.hpp"

#include "packet/bpf/bpf_parse.hpp"

using namespace openperf::packet::bpf;

TEST_CASE("bpf_parse", "[bpf]")
{
    SECTION("good")
    {
        REQUIRE(bpf_parse_string("length == 1000")->to_string()
                == "(length == 1000)");
        REQUIRE(bpf_parse_string("not length == 1000")->to_string()
                == "not(length == 1000)");
        REQUIRE(bpf_parse_string("ip dst 10.0.0.1")->to_string()
                == "(ip dst 10.0.0.1)");
        REQUIRE(
            bpf_parse_string("ip dst 10.0.0.1 and ip src 10.0.0.2")->to_string()
            == "((ip dst 10.0.0.1) && (ip src 10.0.0.2))");
        REQUIRE(
            bpf_parse_string("ip dst 10.0.0.1 or ip src 10.0.0.2")->to_string()
            == "((ip dst 10.0.0.1) || (ip src 10.0.0.2))");
        REQUIRE(bpf_parse_string(
                    "ip src 10.0.0.1 or ip src 10.0.0.2 or ip src 10.0.0.3")
                    ->to_string()
                == "(((ip src 10.0.0.1) || (ip src 10.0.0.2)) || (ip src "
                   "10.0.0.3))");
        REQUIRE(bpf_parse_string("signature streamid 1000")->to_string()
                == "(signature streamid 1000)");
        REQUIRE(bpf_parse_string("signature streamid 1000-2000")->to_string()
                == "(signature streamid 1000-2000)");
        REQUIRE(bpf_parse_string(
                    "signature and (length >= 1000 and length <= 1500)")
                    ->to_string()
                == "((signature) && ((length >= 1000) && (length <= 1500)))");
        REQUIRE(bpf_parse_string("valid fcs")->to_string() == "(valid fcs)");
        REQUIRE(bpf_parse_string("valid chksum")->to_string()
                == "(valid chksum)");
        REQUIRE(bpf_parse_string("valid prbs")->to_string() == "(valid prbs)");
        REQUIRE(bpf_parse_string("valid fcs chksum prbs")->to_string()
                == "(valid fcs chksum prbs)");
        REQUIRE(bpf_parse_string("signature and length == 1000")->to_string()
                == "((signature) && (length == 1000))");
        REQUIRE(
            bpf_parse_string("not signature and length == 1000")->to_string()
            == "(not(signature) && (length == 1000))");
        REQUIRE(bpf_parse_string("not((signature||valid fcs)||length<1000) ")
                    ->to_string()
                == "not(((signature) || (valid fcs)) || (length < 1000))");
    }

    SECTION("bad")
    {
        REQUIRE_THROWS_AS(bpf_parse_string("(length == 1000"),
                          std::runtime_error);
        REQUIRE_THROWS_AS(bpf_parse_string("length == 1000)"),
                          std::runtime_error);
    }
}

TEST_CASE("bpf_split_special", "[bpf]")
{
    SECTION("good")
    {
        REQUIRE(
            bpf_split_special(bpf_parse_string("length == 1000"))->to_string()
            == "(length == 1000)");
        REQUIRE(bpf_split_special(bpf_parse_string("signature"))->to_string()
                == "(signature)");
        REQUIRE(bpf_split_special(bpf_parse_string("not not length == 1000"))
                    ->to_string()
                == "(length == 1000)");
        REQUIRE(bpf_split_special(bpf_parse_string("not not signature"))
                    ->to_string()
                == "(signature)");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("not not signature and length == 1000"))
                    ->to_string()
                == "((signature) && (length == 1000))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string(
                        "signature and ip src 1.0.0.1 and ip dst 1.0.0.2"))
                    ->to_string()
                == "((signature) && ((ip src 1.0.0.1) && (ip dst 1.0.0.2)))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("signature streamid 1000-2000 and "
                                     "valid fcs and length == 1000"))
                    ->to_string()
                == "(((signature streamid 1000-2000) && (valid fcs)) && "
                   "(length == 1000))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("signature and not valid prbs and "
                                     "ip src 1.0.0.1 and ip dst 1.0.0.2"))
                    ->to_string()
                == "(((signature) && not(valid prbs)) && ((ip src 1.0.0.1) && "
                   "(ip dst 1.0.0.2)))");
        REQUIRE(
            bpf_split_special(bpf_parse_string("length == 1000 and signature"))
                ->to_string()
            == "((signature) && (length == 1000))");
        REQUIRE(
            bpf_split_special(
                bpf_parse_string("length == 1000 and signature and valid fcs"))
                ->to_string()
            == "(((signature) && (valid fcs)) && (length == 1000))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string(
                        "signature and (length > 1000 and length < 1500)"))
                    ->to_string()
                == "((signature) && ((length > 1000) && (length < 1500)))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string(
                        "(length > 1000 and length < 1500) and ((signature))"))
                    ->to_string()
                == "((signature) && ((length > 1000) && (length < 1500)))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("(length > 1000 and length < 1500) and "
                                     "(signature streamid 1000 and valid fcs)"))
                    ->to_string()
                == "(((signature streamid 1000) && (valid fcs)) && "
                   "((length > 1000) && (length < 1500)))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string(
                        "(length > 1000 and length < 1500) and "
                        "(not signature streamid 1000 and not valid fcs)"))
                    ->to_string()
                == "((not(signature streamid 1000) && not(valid fcs)) && "
                   "((length > 1000) && (length < 1500)))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("not(signature && length < 1000)"))
                    ->to_string()
                == "(not(signature) || not(length < 1000))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("not(signature || length < 1000)"))
                    ->to_string()
                == "(not(signature) && not(length < 1000))");
        REQUIRE(bpf_split_special(
                    bpf_parse_string("not(signature||valid fcs||length<1000)"))
                    ->to_string()
                == "(not((signature) || (valid fcs)) && not(length < 1000))");
    }

    SECTION("bad")
    {
        REQUIRE_THROWS_AS(
            bpf_split_special(bpf_parse_string(
                "signature || (not(signature) && length < 1000)")),
            std::runtime_error);
        REQUIRE_THROWS_AS(
            bpf_split_special(bpf_parse_string(
                "signature || (not(valid fcs) && length < 1000)")),
            std::runtime_error);
    }
}
