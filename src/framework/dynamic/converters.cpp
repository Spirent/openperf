#include "api.hpp"

#include <unordered_map>
#include <functional>

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

dynamic::argument_t::function_t to_argument_function(const std::string& value)
{
    static const std::unordered_map<std::string,
                                    dynamic::argument_t::function_t>
        smap = {
            {"dx", dynamic::argument_t::DX},
            {"dxdy", dynamic::argument_t::DXDY},
            {"dxdt", dynamic::argument_t::DXDT},
        };

    if (smap.count(value)) return smap.at(value);
    throw std::runtime_error("Function \"" + value + "\" is unknown");
}

std::string to_string(const dynamic::argument_t::function_t& f)
{
    static const std::unordered_map<dynamic::argument_t::function_t,
                                    std::string>
        fmap = {
            {dynamic::argument_t::DX, "dx"},
            {dynamic::argument_t::DXDY, "dxdy"},
            {dynamic::argument_t::DXDT, "dxdt"},
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
        .id = m.getId()};
}

configuration from_swagger(model::DynamicResultsConfig& m)
{
    configuration config{};

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

model::ThresholdResult to_swagger(const dynamic::results::threshold& r)
{
    model::ThresholdResult result;

    result.setId(r.id);
    result.setFunction(to_string(r.argument.function));
    result.setStatX(r.argument.x);

    if (r.argument.function == dynamic::argument_t::DXDY)
        result.setStatY(r.argument.y);

    result.setValue(r.threshold.value());
    result.setCondition(to_string(r.threshold.condition()));
    result.setConditionTrue(r.threshold.trues());
    result.setConditionFalse(r.threshold.falses());

    return result;
}

model::DynamicResults to_swagger(const dynamic::results& r)
{
    auto vector = std::vector<std::shared_ptr<model::ThresholdResult>>();
    std::transform(r.thresholds.begin(),
                   r.thresholds.end(),
                   std::back_inserter(vector),
                   [](const auto& x) {
                       return std::make_shared<model::ThresholdResult>(
                           to_swagger(x));
                   });

    model::DynamicResults result;
    result.getThresholds() = vector;
    return result;
}

} // namespace openperf::dynamic
