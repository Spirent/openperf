#ifndef _OP_DYNAMIC_API_HPP_
#define _OP_DYNAMIC_API_HPP_

#include <string>
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
        argument_t argument;
        std::string id;
        double value;
        comparator condition;
    };

    struct tdigest
    {
        argument_t argument;
        std::string id;
        uint32_t compression;
    };

    std::vector<threshold> thresholds;
    std::vector<tdigest> tdigests;
};

// Enum Conversions
comparator to_comparator(const std::string&);
argument_t::function_t to_argument_function(const std::string&);
std::string to_string(const comparator&);
std::string to_string(const argument_t::function_t&);

// Swagger Model Conversions
configuration from_swagger(model::DynamicResultsConfig&);
configuration::tdigest from_swagger(const model::TDigestConfig&);
configuration::threshold from_swagger(const model::ThresholdConfig&);
model::DynamicResults to_swagger(const results&);
model::TDigestResult to_swagger(const results::tdigest&);
model::ThresholdResult to_swagger(const results::threshold&);

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_API_HPP_
