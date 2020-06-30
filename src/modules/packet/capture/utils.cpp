#include "packet/capture/api.hpp"

#include "packet/bpf/bpf.hpp"
#include "swagger/v1/model/PacketCapture.h"

namespace openperf::packet::capture::api {

bool is_valid(const capture_type& capture, std::vector<std::string>& errors)
{
    auto init_errors = errors.size();

    if (capture.getSourceId().empty()) {
        errors.emplace_back("Capture source id is required.");
    }

    auto config = capture.getConfig();
    if (!config) {
        errors.emplace_back("Capture configuration is required.");
        return (false);
    }

    if (config->getBufferSize()
        < static_cast<int64_t>(capture_buffer_size_min)) {
        errors.emplace_back(
            "Capture buffer size " + std::to_string(config->getBufferSize())
            + " is less than minimum allowed "
            + std::to_string(capture_buffer_size_min) + " bytes.");
    }

    if (config->filterIsSet()) {
        auto filter = config->getFilter();
        if (!packet::bpf::bpf_validate_filter(filter)) {
            errors.emplace_back("Capture filter (" + filter
                                + ") is not valid.");
        }
    }

    if (config->startTriggerIsSet()) {
        auto start_trigger = config->getStartTrigger();
        if (!bpf::bpf_validate_filter(start_trigger)) {
            errors.emplace_back("Capture start trigger (" + start_trigger
                                + ") is not valid.");
        }
    }

    if (config->stopTriggerIsSet()) {
        auto stop_trigger = config->getStopTrigger();
        if (!bpf::bpf_validate_filter(stop_trigger)) {
            errors.emplace_back("Capture stop trigger (" + stop_trigger
                                + ") is not valid.");
        }
    }

    return (errors.size() == init_errors);
}

} // namespace openperf::packet::capture::api
