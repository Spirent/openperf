#ifndef _OP_DYNAMIC_API_HPP_
#define _OP_DYNAMIC_API_HPP_

#include <string>
#include <vector>

#include "dynamic/threshold.hpp"
#include "digestible/digestible.h"

#include "swagger/v1/model/DynamicResultsConfig.h"
#include "swagger/v1/model/DynamicResults.h"
#include "swagger/v1/model/ThresholdConfig.h"
#include "swagger/v1/model/ThresholdResult.h"
#include "swagger/v1/model/TDigestConfig.h"
#include "swagger/v1/model/TDigestResult.h"

namespace openperf::dynamic {

namespace model = ::swagger::v1::model;

using tdigest = digestible::tdigest<double, uint32_t>;
using centroid = digestible::centroid<double, uint32_t>;

struct argument_t
{
    enum function_t { DX = 1, DXDY, DXDT };
    std::string x;
    std::string y;
    function_t function;
};

struct results
{
    using tdigest_ptr = std::shared_ptr<tdigest>;

    struct threshold
    {
        dynamic::argument_t argument;
        std::string id;
        dynamic::threshold threshold;
    };

    struct tdigest
    {
        dynamic::argument_t argument;
        std::string id;
        uint32_t compression;
        tdigest_ptr tdigest;
    };

    std::vector<results::threshold> thresholds;
    std::vector<results::tdigest> tdigests;
};

struct configuration
{
    struct threshold
    {
        dynamic::argument_t argument;
        std::string id;
        double value;
        comparator condition;
    };

    struct tdigest
    {
        dynamic::argument_t argument;
        std::string id;
        uint32_t compression;
    };

    std::vector<threshold> thresholds;
    std::vector<tdigest> tdigests;
};

// Enum Conversions
comparator to_comparator(const std::string&);
dynamic::argument_t::function_t to_argument_function(const std::string&);
std::string to_string(const comparator&);
std::string to_string(const dynamic::argument_t::function_t&);

// Swagger Model Conversions
configuration from_swagger(model::DynamicResultsConfig&);
configuration::tdigest from_swagger(const model::TDigestConfig&);
configuration::threshold from_swagger(const model::ThresholdConfig&);
model::DynamicResults to_swagger(const dynamic::results&);
model::TDigestCentroid to_swagger(const dynamic::centroid&);
model::TDigestResult to_swagger(const dynamic::results::tdigest&);
model::ThresholdResult to_swagger(const dynamic::results::threshold&);

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_API_HPP_
