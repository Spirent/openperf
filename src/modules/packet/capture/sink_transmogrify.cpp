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
    config->setMode(to_string(src.get_capture_mode()));
    config->setBufferWrap(src.get_buffer_wrap());
    if (auto packet_size = src.get_max_packet_size(); packet_size != UINT32_MAX)
        config->setPacketSize(packet_size);
    if (auto duration = src.get_duration()) config->setDuration(duration);
    if (auto filter = src.get_filter_str(); !filter.empty())
        config->setFilter(filter);
    if (auto trigger = src.get_start_trigger_str(); !trigger.empty())
        config->setStartTrigger(trigger);
    if (auto trigger = src.get_stop_trigger_str(); !trigger.empty())
        config->setStopTrigger(trigger);
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
    auto stats = src.get_stats();
    dst->setPackets(stats.packets);
    dst->setBytes(stats.bytes);

    return (dst);
}

} // namespace openperf::packet::capture::api
