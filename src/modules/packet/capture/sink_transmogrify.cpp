#include <iomanip>
#include <sstream>

#include "core/op_uuid.hpp"
#include "packet/capture/api.hpp"
#include "packet/capture/sink.hpp"

#include "swagger/v1/model/PacketCapture.h"
#include "swagger/v1/model/PacketCaptureResult.h"

namespace openperf::packet::capture::api {

capture_ptr to_swagger(const sink& src)
{
    auto dst = std::make_unique<swagger::v1::model::PacketCapture>();

    dst->setId(src.id());
    dst->setSourceId(src.source());
    dst->setActive(src.active());

    auto config = std::make_shared<swagger::v1::model::PacketCaptureConfig>();
    dst->setConfig(config);

    return (dst);
}

capture_result_ptr to_swagger(const core::uuid& id, const sink_result& src)
{
    auto dst = std::make_unique<swagger::v1::model::PacketCaptureResult>();

    dst->setId(core::to_string(id));
    dst->setCaptureId(src.parent.id());
    dst->setActive(src.parent.active());

    dst->setState(to_string(src.state.load(std::memory_order_consume)));
    dst->setPackets(src.packets);
    dst->setBytes(src.bytes);

    return (dst);
}

} // namespace openperf::packet::capture::api
