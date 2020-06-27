#ifndef _OP_DYNAMIC_API_HPP_
#define _OP_DYNAMIC_API_HPP_

#include <string>
#include <string_view>
#include <memory>
#include <vector>

#include "dynamic/threshold.hpp"

namespace swagger::v1::model {
class DynamicResultsConfig;
class DynamicResults;
class ThresholdConfig;
class ThresholdResult;
class TDigestConfig;
class TDigestResult;
} // namespace swagger::v1::model

namespace openperf::dynamic {

namespace model = ::swagger::v1::model;

struct argument_t
{
    enum function_t { DX = 1, DXDY, DXDT };
    std::string x;
    std::string y;
    function_t function;
};

struct results
{
    using centroid = std::pair<double, uint32_t>;

    struct threshold
    {
        argument_t argument;
        std::string id;
        dynamic::threshold threshold;
    };

    struct tdigest
    {
        argument_t argument;
        std::string id;
        uint32_t compression;
        std::vector<centroid> centroids;
    };

    std::vector<results::threshold> thresholds;
    std::vector<results::tdigest> tdigests;
};

struct configuration
{
    struct threshold
    {
<<<<<<< HEAD
        argument_t argument;
=======
        dynamic_argument argument;
>>>>>>> bb25ad94... Add Dynamic Framework
        std::string id;
        double value;
        comparator condition;
    };

    struct tdigest
    {
<<<<<<< HEAD
        argument_t argument;
        std::string id;
        uint32_t compression;
=======
        dynamic_argument argument;
        std::string id;
>>>>>>> bb25ad94... Add Dynamic Framework
    };

    std::vector<threshold> thresholds;
    std::vector<tdigest> tdigests;
};

// Enum Conversions
<<<<<<< HEAD
constexpr comparator to_comparator(std::string_view);
constexpr argument_t::function_t to_argument_function(std::string_view);
constexpr std::string_view to_string(const comparator&);
constexpr std::string_view to_string(const argument_t::function_t&);

// Swagger Model Conversions
configuration from_swagger(model::DynamicResultsConfig&);
configuration::tdigest from_swagger(const model::TDigestConfig&);
configuration::threshold from_swagger(const model::ThresholdConfig&);
model::DynamicResults to_swagger(const results&);
model::TDigestResult to_swagger(const results::tdigest&);
model::ThresholdResult to_swagger(const results::threshold&);
=======
inline comparator to_comparator(const std::string& value)
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

inline std::string to_string(const comparator& pattern)
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

inline dynamic_argument::function_t
to_dynamic_function(const std::string& value)
{
    static const std::unordered_map<std::string, dynamic_argument::function_t>
        smap = {
            {"dx", dynamic_argument::DX},
            {"dxdy", dynamic_argument::DXDY},
            {"dxdt", dynamic_argument::DXDT},
        };

    if (smap.count(value)) return smap.at(value);
    throw std::runtime_error("Function \"" + value + "\" is unknown");
}

inline std::string to_string(const dynamic_argument::function_t& f)
{
    static const std::unordered_map<dynamic_argument::function_t, std::string>
        fmap = {
            {dynamic_argument::DX, "dx"},
            {dynamic_argument::DXDY, "dxdy"},
            {dynamic_argument::DXDT, "dxdt"},
        };

    if (fmap.count(f)) return fmap.at(f);
    return "unknown";
}

// Swagger Model Conversions
inline configuration::threshold from_swagger(const model::ThresholdConfig& m)
{
    return {.argument = {.x = m.getStatX(),
                         .y = m.getStatY(),
                         .function = to_dynamic_function(m.getFunction())},
            .id = m.getId(),
            .value = m.getValue(),
            .condition = to_comparator(m.getCondition())};
}

inline configuration::tdigest from_swagger(const model::TDigestConfig& m)
{
    return {.argument = {.x = m.getStatX(),
                         .y = m.getStatY(),
                         .function = to_dynamic_function(m.getFunction())},
            .id = m.getId()};
}

inline configuration from_swagger(model::DynamicResultsConfig& m)
{
    configuration config{};

    const auto& thresholds = m.getThresholds();
    std::transform(thresholds.begin(),
                   thresholds.end(),
                   std::back_inserter(config.thresholds),
                   [](const auto& x) -> configuration::threshold {
                       return from_swagger(*x.get());
                   });

    const auto& tdigests = m.getTdigests();
    std::transform(tdigests.begin(),
                   tdigests.end(),
                   std::back_inserter(config.tdigests),
                   [](const auto& x) -> configuration::tdigest {
                       return from_swagger(*x.get());
                   });

    return config;
}

inline model::ThresholdResult
to_swagger(const dynamic::results::threshold_result& r)
{
    model::ThresholdResult result;

    result.setId(r.id);
    result.setFunction(to_string(r.argument.function));
    result.setStatX(r.argument.x);

    if (r.argument.function == dynamic_argument::DXDY)
        result.setStatY(r.argument.y);

    result.setValue(r.threshold.value());
    result.setCondition(to_string(r.threshold.condition()));
    result.setConditionTrue(r.threshold.trues());
    result.setConditionFalse(r.threshold.falses());

    return result;
}

inline model::DynamicResults to_swagger(const dynamic::results& r)
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
>>>>>>> bb25ad94... Add Dynamic Framework

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_API_HPP_
