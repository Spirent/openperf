#include "api.hpp"

#include <unordered_map>
#include <functional>

#include "swagger/v1/model/DynamicResultsConfig.h"
#include "swagger/v1/model/DynamicResults.h"
#include "swagger/v1/model/ThresholdConfig.h"
#include "swagger/v1/model/ThresholdResult.h"
#include "swagger/v1/model/TDigestConfig.h"
#include "swagger/v1/model/TDigestResult.h"
#include "swagger/v1/model/TDigestCentroid.h"

namespace openperf::dynamic {

// Enum Conversions
constexpr comparator to_comparator(std::string_view value)
{
    if (value == "equal") return comparator::EQUAL;
    if (value == "greater") return comparator::GREATER_THAN;
    if (value == "greater_or_equal") return comparator::GREATER_OR_EQUAL;
    if (value == "less") return comparator::LESS_THAN;
    if (value == "less_or_equal") return comparator::LESS_OR_EQUAL;

    throw std::runtime_error(
        "Error from string to comparator conversion: illegal string value");
}

constexpr std::string_view to_string(const comparator& pattern)
{
    switch (pattern) {
    case comparator::EQUAL:
        return "equal";
    case comparator::GREATER_THAN:
        return "greater";
    case comparator::GREATER_OR_EQUAL:
        return "greater_or_equal";
    case comparator::LESS_THAN:
        return "less";
    case comparator::LESS_OR_EQUAL:
        return "less_or_equal";
    default:
        return "unknown";
    };
}

constexpr argument_t::function_t to_argument_function(std::string_view value)
{
    if (value == "dx") return argument_t::DX;
    if (value == "dxdy") return argument_t::DXDY;
    if (value == "dxdt") return argument_t::DXDT;

    throw std::runtime_error(
        "Error from string to function_t conversion: illegal string value");
}

constexpr std::string_view to_string(const argument_t::function_t& value)
{
    switch (value) {
    case argument_t::DX:
        return "dx";
    case argument_t::DXDY:
        return "dxdy";
    case argument_t::DXDT:
        return "dxdt";
    default:
        return "unknown";
    };
}

// Swagger Model Conversions
configuration::threshold from_swagger(const model::ThresholdConfig& m)
{
    return configuration::threshold{
        .argument = {.x = m.getStatX(),
                     .y = m.getStatY(),
                     .function = to_argument_function(m.getFunction())},
        .id = m.getId(),
        .value = m.getValue(),
        .condition = to_comparator(m.getCondition())};
}

configuration::tdigest from_swagger(const model::TDigestConfig& m)
{
    return configuration::tdigest{
        .argument = {.x = m.getStatX(),
                     .y = m.getStatY(),
                     .function = to_argument_function(m.getFunction())},
        .id = m.getId(),
        .compression = static_cast<uint32_t>(m.getCompression())};
}

configuration from_swagger(model::DynamicResultsConfig& m)
{
    configuration config;

    const auto& thresholds = m.getThresholds();
    std::transform(thresholds.begin(),
                   thresholds.end(),
                   std::back_inserter(config.thresholds),
                   [](const auto& x) -> configuration::threshold {
                       return from_swagger(*x);
                   });

    const auto& tdigests = m.getTdigests();
    std::transform(tdigests.begin(),
                   tdigests.end(),
                   std::back_inserter(config.tdigests),
                   [](const auto& x) -> configuration::tdigest {
                       return from_swagger(*x);
                   });

    return config;
}

model::ThresholdResult to_swagger(const results::threshold& r)
{
    model::ThresholdResult result;

    result.setId(r.id);
    result.setFunction(std::string(to_string(r.argument.function)));
    result.setStatX(r.argument.x);

    if (r.argument.function == argument_t::DXDY) result.setStatY(r.argument.y);

    auto& threshold = r.threshold;
    result.setValue(threshold.value());
    result.setCondition(std::string(to_string(threshold.condition())));
    result.setConditionTrue(threshold.trues());
    result.setConditionFalse(threshold.falses());

    return result;
}

model::TDigestResult to_swagger(const results::tdigest& r)
{
    model::TDigestResult result;

    result.setId(r.id);
    result.setFunction(std::string(to_string(r.argument.function)));
    result.setStatX(r.argument.x);

    if (r.argument.function == argument_t::DXDY) result.setStatY(r.argument.y);

    result.setCompression(r.compression);

    auto centroids = r.centroids;
    std::transform(centroids.begin(),
                   centroids.end(),
                   std::back_inserter(result.getCentroids()),
                   [](const auto& x) {
                       model::TDigestCentroid centroid;
                       centroid.setMean(x.first);
                       centroid.setWeight(x.second);

                       return std::make_shared<model::TDigestCentroid>(
                           centroid);
                   });

    return result;
}

model::DynamicResults to_swagger(const results& r)
{
    model::DynamicResults result;

    auto vector = std::vector<std::shared_ptr<model::ThresholdResult>>();
    std::transform(r.thresholds.begin(),
                   r.thresholds.end(),
                   std::back_inserter(result.getThresholds()),
                   [](const auto& x) {
                       return std::make_shared<model::ThresholdResult>(
                           to_swagger(x));
                   });

    std::transform(r.tdigests.begin(),
                   r.tdigests.end(),
                   std::back_inserter(result.getTdigests()),
                   [](const auto& x) {
                       return std::make_shared<model::TDigestResult>(
                           to_swagger(x));
                   });

    return result;
}

} // namespace openperf::dynamic
