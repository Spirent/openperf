#ifndef _OP_PACKETIO_JSON_TRANSMOGRIFY_H_
#define _OP_PACKETIO_JSON_TRANSMOGRIFY_H_

#include "json.hpp"
#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Port.h"

namespace swagger {
namespace v1 {
namespace model {

void from_json(const nlohmann::json&, swagger::v1::model::Interface&);
void from_json(const nlohmann::json&, swagger::v1::model::Port&);

}
}
}

namespace openperf {
namespace packetio {
    static const std::string empty_id_string = "none";
}
}

#endif /* _OP_PACKETIO_JSON_TRANSMOGRIFY_H_ */
