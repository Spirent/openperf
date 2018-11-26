#include <cassert>

#include "swagger/v1/model/Stack.h"
#include "packetio/generic_stack.h"

namespace icp {
namespace packetio {
namespace stack {

using namespace swagger::v1::model;

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

typedef void(protocol_getter)(std::shared_ptr<StackProtocolStats>);

static std::shared_ptr<StackProtocolStats>
make_swagger_protocol_stats(const protocol_stats_data& src)
{
    auto dst = std::make_shared<StackProtocolStats>();

    dst->setTxPackets(src.tx_packets);
    dst->setRxPackets(src.rx_packets);
    dst->setForwardedPackets(src.forwarded_packets);
    dst->setDroppedPackets(src.dropped_packets);
    dst->setChecksumErrors(src.checksum_errors);
    dst->setLengthErrors(src.length_errors);
    dst->setMemoryErrors(src.memory_errors);
    dst->setRoutingErrors(src.routing_errors);
    dst->setProtocolErrors(src.protocol_errors);
    dst->setOptionErrors(src.option_errors);
    dst->setMiscErrors(src.misc_errors);
    dst->setCacheHits(src.cache_hits);

    return (dst);
}

static std::shared_ptr<StackMemoryStats>
make_swagger_memory_stats(const memory_stats_data& src)
{
    auto dst = std::make_shared<StackMemoryStats>();

    dst->setName(src.name);
    dst->setAvailable(src.available);
    dst->setUsed(src.used);
    dst->setMax(src.max);
    dst->setErrors(src.errors);
    dst->setIllegal(src.illegal);

    return (dst);
}

static std::shared_ptr<StackElementStats>
make_swagger_element_stats(const element_stats_data& src)
{
    auto dst = std::make_shared<StackElementStats>();

    dst->setUsed(src.used);
    dst->setMax(src.max);
    dst->setErrors(src.errors);

    return (dst);
}

static std::shared_ptr<StackStats> make_swagger_stack_stats(const generic_stack& stack)
{
    auto stats = std::make_shared<StackStats>();

    /**
     * Whew.  Simplify some names to make this gobbledygook a little easier
     * to understand.
     */
    using element_setter = std::function<void(std::shared_ptr<StackElementStats>)>;
    using protocol_setter = std::function<void(std::shared_ptr<StackProtocolStats>)>;
    using generic_setter = std::variant<element_setter, protocol_setter>;
    using setters_map = std::unordered_map<std::string, generic_setter>;
    using std::placeholders::_1;

    /**
     * Provide a mapping between statistics keys and stats object setter functions.
     * Unfortunately, this can't be static map because we need to bind to the current
     * instance of the stats object.
     */
    setters_map setters = {
        {"sems",      std::bind(&StackStats::setSems,     stats, _1) },
        {"mutexes",   std::bind(&StackStats::setMutexes,  stats, _1) },
        {"mboxes",    std::bind(&StackStats::setMboxes,   stats, _1) },
        {"arp",       std::bind(&StackStats::setArp,      stats, _1) },
        {"ipv4",      std::bind(&StackStats::setIpv4,     stats, _1) },
        {"ipv4_frag", std::bind(&StackStats::setIpv4Frag, stats, _1) },
        {"icmpv4",    std::bind(&StackStats::setIcmpv4,   stats, _1) },
        {"igmp",      std::bind(&StackStats::setIgmp,     stats, _1) },
        {"nd",        std::bind(&StackStats::setNd,       stats, _1) },
        {"ipv6",      std::bind(&StackStats::setIpv6,     stats, _1) },
        {"ipv6_frag", std::bind(&StackStats::setIpv6Frag, stats, _1) },
        {"icmpv6",    std::bind(&StackStats::setIcmpv6,   stats, _1) },
        {"mld",       std::bind(&StackStats::setMld,      stats, _1) },
        {"udp",       std::bind(&StackStats::setUdp,      stats, _1) },
        {"tcp",       std::bind(&StackStats::setTcp,      stats, _1) }
    };

    /* One of the functions is not like the others... */
    for (auto& kv : stack.stats()) {
        std::visit(overloaded_visitor(
                       [&](const element_stats_data& element) {
                           std::get<element_setter>(setters[kv.first])(make_swagger_element_stats(element));
                       },
                       [&](const memory_stats_data& memory) {
                           stats->getPools().emplace_back(make_swagger_memory_stats(memory));
                       },
                       [&](const protocol_stats_data& protocol) {
                           std::get<protocol_setter>(setters[kv.first])(make_swagger_protocol_stats(protocol));
                       }),
                   kv.second);
    }

    return (stats);
}

std::shared_ptr<Stack> make_swagger_stack(const generic_stack& in_stack)
{
    auto out_stack = std::make_shared<Stack>();

    out_stack->setId(std::to_string(in_stack.id()));
    out_stack->setStats(make_swagger_stack_stats(in_stack));

    return (out_stack);
}

}
}
}
