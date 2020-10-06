#ifndef _OP_SWAGGER_CONVERTERS_PACKET_STACK_HPP_
#define _OP_SWAGGER_CONVERTERS_PACKET_STACK_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class Interface;

void from_json(const nlohmann::json&, Interface&);

} // namespace swagger::v1::model

#endif /* _OP_SWAGGER_CONVERTERS_PACKET_STACK_HPP_ */
