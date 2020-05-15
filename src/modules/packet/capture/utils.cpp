#include "packet/capture/api.hpp"

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

    assert(config);

    return (errors.size() == init_errors);
}

} // namespace openperf::packet::capture::api
