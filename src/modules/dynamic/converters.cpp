#include <algorithm>
#include <cctype>

#include "dynamic/api.hpp"
#include "utils/associative_array.hpp"

#include "swagger/v1/model/DynamicResultsConfig.h"
#include "swagger/v1/model/DynamicResults.h"

namespace openperf::dynamic {

// Enum Conversions
constexpr auto comparator_type_names =
    utils::associative_array<std::string_view, comparator>(
        std::pair("equal", comparator::EQUAL),
        std::pair("greater", comparator::GREATER_THAN),
        std::pair("greater_or_equal", comparator::GREATER_OR_EQUAL),
        std::pair("less", comparator::LESS_THAN),
        std::pair("less_or_equal", comparator::LESS_OR_EQUAL));

constexpr auto function_type_names =
    utils::associative_array<std::string_view, argument_t::function_t>(
        std::pair("dx", argument_t::DX),
        std::pair("dxdy", argument_t::DXDY),
        std::pair("dxdt", argument_t::DXDT));

comparator to_comparator(std::string_view value)
{
    return (utils::key_to_value(comparator_type_names, value)
                .value_or(comparator::NONE));
}

std::string_view to_string(comparator pattern)
{
    return (utils::value_to_key(comparator_type_names, pattern)
                .value_or("unknown"));
}

argument_t::function_t to_argument_function(std::string_view value)
{
    return (utils::key_to_value(function_type_names, value)
                .value_or(argument_t::NONE));
}

std::string_view to_string(argument_t::function_t value)
{
    return (
        utils::value_to_key(function_type_names, value).value_or("unknown"));
}

std::string normalize(std::string_view input)
{
    auto output = std::string{};

    /* Upper -> Lower */
    std::transform(std::begin(input),
                   std::end(input),
                   std::back_inserter(output),
                   [](unsigned char c) { return (std::tolower(c)); });

    /* Remove spaces; use unsigned char for safety */
    output.erase(
        std::remove_if(std::begin(output),
                       std::end(output),
                       [](unsigned char c) { return (std::isspace(c)); }),
        std::end(output));

    return (output);
}

// Swagger Model Conversions
configuration::threshold from_swagger(const model::ThresholdConfig& m)
{
    return configuration::threshold{
        .argument = {.x = normalize(m.getStatX()),
                     .y = normalize(m.getStatY()),
                     .function = to_argument_function(m.getFunction())},
        .id = m.getId(),
        .value = m.getValue(),
        .condition = to_comparator(m.getCondition())};
}

configuration::tdigest from_swagger(const model::TDigestConfig& m)
{
    return configuration::tdigest{
        .argument = {.x = normalize(m.getStatX()),
                     .y = normalize(m.getStatY()),
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
