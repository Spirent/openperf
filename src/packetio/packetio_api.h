#include <string>

#include "pistache/http_defs.h"

namespace icp {
namespace packetio {

enum class Request { ListPorts, CreatePort, GetPort, UpdatePort, DeletePort };

template<typename T>
struct api_request {
    Request operation_id;
    T parameter;
};

struct api_reply {
    Pistache::Http::Code code;
    std::string data;
};

extern const std::string api_endpoint;

}
}
