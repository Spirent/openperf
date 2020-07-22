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
comparator to_comparator(const std::string& value)
{
    static const std::unordered_map<std::string, comparator> smap = {
        {"equal", comparator::EQUAL},
        {"greater", comparator::GREATER_THAN},
        {"greater_or_equal", comparator::GREATER_OR_EQUAL},
        {"less", comparator::LESS_THAN},
        {"less_or_equal", comparator::LESS_OR_EQUAL},
    };

    if (smap.count(value)) return smap.at(value);
    throw std::runtime_error("Condition \"" + value + "\" is unknown");
}

std::string to_string(const comparator& pattern)
{
    static const std::unordered_map<comparator, std::string> fmap = {
        {comparator::EQUAL, "equal"},
        {comparator::GREATER_THAN, "greater"},
        {comparator::GREATER_OR_EQUAL, "greater_or_equal"},
        {comparator::LESS_THAN, "less"},
        {comparator::LESS_OR_EQUAL, "less_or_equal"},
    };

    if (fmap.count(pattern)) return fmap.at(pattern);
    return "unknown";
}

argument_t::function_t to_argument_function(const std::string& value)
{
    static const std::unordered_map<std::string, argument_t::function_t> smap =
        {
            {"dx", argument_t::DX},
            {"dxdy", argument_t::DXDY},
            {"dxdt", argument_t::DXDT},
        };

    if (smap.count(value)) return smap.at(value);
    throw std::runtime_error("Function \"" + value + "\" is unknown");
}

std::string to_string(const argument_t::function_t& f)
{
    static const std::unordered_map<argument_t::function_t, std::string> fmap =
        {
            {argument_t::DX, "dx"},
            {argument_t::DXDY, "dxdy"},
            {argument_t::DXDT, "dxdt"},
        };

    if (fmap.count(f)) return fmap.at(f);
    return "unknown";
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
    result.setFunction(to_string(r.argument.function));
    result.setStatX(r.argument.x);

    if (r.argument.function == argument_t::DXDY) result.setStatY(r.argument.y);

    auto& threshold = r.threshold;
    result.setValue(threshold.value());
    result.setCondition(to_string(threshold.condition()));
    result.setConditionTrue(threshold.trues());
    result.setConditionFalse(threshold.falses());

    return result;
}

model::TDigestResult to_swagger(const results::tdigest& r)
{
    model::TDigestResult result;

    result.setId(r.id);
    result.setFunction(to_string(r.argument.function));
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
