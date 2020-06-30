#include "packet/analyzer/api.hpp"

#include "packet/analyzer/statistics/generic_flow_counters.hpp"
#include "packet/statistics/generic_protocol_counters.hpp"
#include "packet/bpf/bpf.hpp"

#include "swagger/v1/model/PacketAnalyzer.h"

namespace openperf::packet::analyzer::api {

bool is_valid(const analyzer_type& analyzer, std::vector<std::string>& errors)
{
    auto init_errors = errors.size();

    if (analyzer.getSourceId().empty()) {
        errors.emplace_back("Analyzer source id is required.");
    }

    auto config = analyzer.getConfig();
    if (!config) {
        errors.emplace_back("Analyzer configuration is required.");
        return (false);
    }

    assert(config);

    for (auto& item : config->getProtocolCounters()) {
        if (packet::statistics::to_protocol_flag(item)
            == packet::statistics::protocol_flags::none) {
            errors.emplace_back("Protocol counter (" + item
                                + ") is not recognized.");
        }
    }

    for (auto& item : config->getFlowCounters()) {
        if (statistics::to_flow_flag(item) == statistics::flow_flags::none) {
            errors.emplace_back("Flow counter (" + item
                                + ") is not recognized.");
        }
    }

    if (config->filterIsSet()) {
        auto filter = config->getFilter();
        if (!bpf::bpf_validate_filter(filter)) {
            errors.emplace_back("Filter (" + filter + ") is not valid.");
        }
    }

    return (errors.size() == init_errors);
}

} // namespace openperf::packet::analyzer::api
