#ifndef _ICP_PACKETIO_JSON_TRANSMOGRIFY_H_
#define _ICP_PACKETIO_JSON_TRANSMOGRIFY_H_

#include "json.hpp"
#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Port.h"

using json = nlohmann::json;

namespace swagger {
namespace v1 {
namespace model {

void from_json(const json &j, Interface &interface);
void from_json(const json &j, Port &port);

}
}
}

#endif /* _ICP_PACKETIO_JSON_TRANSMOGRIFY_H_ */
