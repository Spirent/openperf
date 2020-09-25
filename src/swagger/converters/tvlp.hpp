#ifndef _OP_TVLP_JSON_CONVERTERS_HPP_
#define _OP_TVLP_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class TvlpConfiguration;

void from_json(const nlohmann::json&, TvlpConfiguration&);

} // namespace swagger::v1::model

#endif