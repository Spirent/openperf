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
    auto& src_config = src.get_config();
    auto dst = std::make_unique<swagger::v1::model::PacketCapture>();

    dst->setId(src.id());
    dst->setSourceId(src.source());
    dst->setDirection(to_string(src.direction()));
    dst->setActive(src.active());

    auto dst_config =
        std::make_shared<swagger::v1::model::PacketCaptureConfig>();
    dst_config->setMode(to_string(src_config.capture_mode));
    dst_config->setBufferWrap(src_config.buffer_wrap);
    if (src_config.max_packet_size != UINT32_MAX)
        dst_config->setPacketSize(src_config.max_packet_size);
    if (src_config.duration.count()) {
        auto duration_msec =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                src_config.duration)
                .count();
        dst_config->setDuration(duration_msec);
    }
    if (src_config.packet_count) {
        dst_config->setPacketCount(src_config.packet_count);
    }
    if (!src_config.filter.empty()) dst_config->setFilter(src_config.filter);
    if (!src_config.start_trigger.empty())
        dst_config->setStartTrigger(src_config.start_trigger);
    if (!src_config.stop_trigger.empty())
        dst_config->setStopTrigger(src_config.stop_trigger);
    dst->setConfig(dst_config);

    return (dst);
}

capture_result_ptr to_swagger(const core::uuid& id, const sink_result& src)
{
    auto dst = std::make_unique<swagger::v1::model::PacketCaptureResult>();

    dst->setId(core::to_string(id));
    dst->setCaptureId(src.parent.id());
    dst->setActive(src.parent.active());

    dst->setState(to_string(src.state.load(std::memory_order_consume)));
    auto stats = src.get_stats();
    dst->setPackets(stats.packets);
    dst->setBytes(stats.bytes);

    return (dst);
}

} // namespace openperf::packet::capture::api
