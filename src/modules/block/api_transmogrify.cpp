#include <zmq.h>
#include <sstream>
#include <iomanip>
#include <netdb.h>
#include "block/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::block::api {

template <typename T> static auto zmq_msg_init(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, std::unique_ptr<T> value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T**>(zmq_msg_data(msg));
    *ptr = value.release();
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy(src, src + length, ptr);
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg,
                         std::vector<std::unique_ptr<T>>& values)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*) * values.size());
        error != 0) {
        return (error);
    }

    auto cursor = reinterpret_cast<T**>(zmq_msg_data(msg));
    std::transform(std::begin(values), std::end(values), cursor, [](auto& ptr) {
        return (ptr.release());
    });
    return (0);
}

template <typename T>
static std::enable_if_t<!std::is_pointer_v<T>, T>
zmq_msg_data(const zmq_msg_t* msg)
{
    return *(reinterpret_cast<T*>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

template <typename T>
static std::enable_if_t<std::is_pointer_v<T>, T>
zmq_msg_data(const zmq_msg_t* msg)
{
    return (reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

static void close(serialized_msg& msg)
{
    zmq_close(&msg.type);
    zmq_close(&msg.data);
}

void copy_string(std::string_view str, char* ch_arr, size_t max_length)
{
    str.copy(ch_arr, max_length - 1);
    ch_arr[std::min(str.length(), max_length - 1)] = '\0';
}

const static std::unordered_map<std::string, model::block_generation_pattern>
    block_generation_patterns = {
        {"random", model::block_generation_pattern::RANDOM},
        {"sequential", model::block_generation_pattern::SEQUENTIAL},
        {"reverse", model::block_generation_pattern::REVERSE},
};

const static std::unordered_map<model::block_generation_pattern, std::string>
    block_generation_pattern_strings = {
        {model::block_generation_pattern::RANDOM, "random"},
        {model::block_generation_pattern::SEQUENTIAL, "sequential"},
        {model::block_generation_pattern::REVERSE, "reverse"},
};

model::block_generation_pattern
block_generation_pattern_from_string(const std::string& value)
{
    if (block_generation_patterns.count(value))
        return block_generation_patterns.at(value);
    throw std::runtime_error("Pattern \"" + value + "\" is unknown");
}

std::string to_string(const model::block_generation_pattern& pattern)
{
    if (block_generation_pattern_strings.count(pattern))
        return block_generation_pattern_strings.at(pattern);
    return "unknown";
}

const static std::unordered_map<std::string, model::file_state>
    block_file_states = {
        {"none", model::file_state::NONE},
        {"init", model::file_state::INIT},
        {"ready", model::file_state::READY},
};

const static std::unordered_map<model::file_state, std::string>
    block_file_state_strings = {
        {model::file_state::NONE, "none"},
        {model::file_state::INIT, "init"},
        {model::file_state::READY, "ready"},
};

model::file_state block_file_state_from_string(const std::string& value)
{
    if (block_file_states.count(value)) return block_file_states.at(value);
    throw std::runtime_error("Pattern \"" + value + "\" is unknown");
}

std::string to_string(const model::file_state& pattern)
{
    if (block_file_state_strings.count(pattern))
        return block_file_state_strings.at(pattern);
    return "unknown";
}

const char* to_string(const api::typed_error& error)
{
    switch (error.type) {
    case api::error_type::NOT_FOUND:
        return ("");
    case api::error_type::ZMQ_ERROR:
        return (zmq_strerror(error.code));
    case api::error_type::CUSTOM_ERROR:
        return error.value;
    default:
        return ("unknown error type");
    }
}

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_block_device_list&) {
                     return (zmq_msg_init(&serialized.data));
                 },
                 [&](const request_block_device& blkdevice) {
                     return (zmq_msg_init(&serialized.data,
                                          blkdevice.id.data(),
                                          blkdevice.id.length()));
                 },
                 [&](const request_block_file_list&) {
                     return (zmq_msg_init(&serialized.data));
                 },
                 [&](const request_block_file& blkfile) {
                     return (zmq_msg_init(&serialized.data,
                                          blkfile.id.data(),
                                          blkfile.id.length()));
                 },
                 [&](request_block_file_add& blkfile) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(blkfile.source)));
                 },
                 [&](const request_block_file_del& blkfile) {
                     return (zmq_msg_init(&serialized.data,
                                          blkfile.id.data(),
                                          blkfile.id.length()));
                 },
                 [&](request_block_file_bulk_add& request) {
                     return (zmq_msg_init(&serialized.data, request.files));
                 },
                 [&](request_block_file_bulk_del& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](const request_block_generator_list&) {
                     return (zmq_msg_init(&serialized.data));
                 },
                 [&](const request_block_generator& blkgenerator) {
                     return (zmq_msg_init(&serialized.data,
                                          blkgenerator.id.data(),
                                          blkgenerator.id.length()));
                 },
                 [&](request_block_generator_add& blkgenerator) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(blkgenerator.source)));
                 },
                 [&](const request_block_generator_del& blkgenerator) {
                     return (zmq_msg_init(&serialized.data,
                                          blkgenerator.id.data(),
                                          blkgenerator.id.length()));
                 },
                 [&](request_block_generator_bulk_add& request) {
                     return (zmq_msg_init(&serialized.data, request.generators));
                 },
                 [&](request_block_generator_bulk_del& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](const request_block_generator_start& blkgenerator) {
                     return (zmq_msg_init(&serialized.data,
                                          blkgenerator.id.data(),
                                          blkgenerator.id.length()));
                 },
                 [&](const request_block_generator_stop& blkgenerator) {
                     return (zmq_msg_init(&serialized.data,
                                          blkgenerator.id.data(),
                                          blkgenerator.id.length()));
                 },
                 [&](request_block_generator_bulk_start& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](request_block_generator_bulk_stop& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](const request_block_generator_result_list&) {
                     return (zmq_msg_init(&serialized.data));
                 },
                 [&](const request_block_generator_result& result) {
                     return (zmq_msg_init(&serialized.data,
                                          result.id.data(),
                                          result.id.length()));
                 },
                 [&](const request_block_generator_result_del& result) {
                     return (zmq_msg_init(&serialized.data,
                                          result.id.data(),
                                          result.id.length()));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](reply_block_devices& reply) {
                     return (zmq_msg_init(&serialized.data, reply.devices));
                 },
                 [&](reply_block_files& reply) {
                     return (zmq_msg_init(&serialized.data, reply.files));
                 },
                 [&](reply_block_generators& reply) {
                     return (zmq_msg_init(&serialized.data, reply.generators));
                 },
                 [&](reply_block_generator_results& reply) {
                     return (zmq_msg_init(&serialized.data, reply.results));
                 },
                 [&](const reply_ok&) {
                     return (zmq_msg_init(&serialized.data, 0));
                 },
                 [&](const reply_error& error) {
                     return (zmq_msg_init(&serialized.data, error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<request_msg, request_block_device_list>(): {
        return (request_block_device_list{});
    }
    case utils::variant_index<request_msg, request_block_device>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_device{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_file_list>(): {
        return (request_block_file_list{});
    }
    case utils::variant_index<request_msg, request_block_file_add>(): {
        auto request = request_block_file_add{};
        request.source.reset(*zmq_msg_data<file_t**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_block_file>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_file{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_file_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_file_del{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_block_file_bulk_add>(): {
       auto size = zmq_msg_size(&msg.data) / sizeof(file_t*);
        auto data = zmq_msg_data<file_t**>(&msg.data);

        auto request = request_block_file_bulk_add{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.files.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg,
                              request_block_file_bulk_del>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(string_t*);
        auto data = zmq_msg_data<string_t**>(&msg.data);

        auto request = request_block_file_bulk_del{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_block_generator_list>(): {
        return (request_block_generator_list{});
    }
    case utils::variant_index<request_msg, request_block_generator_add>(): {
        auto request = request_block_generator_add{};
        request.source.reset(*zmq_msg_data<generator_t**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_block_generator>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_generator_del{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_bulk_add>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(generator_t*);
        auto data = zmq_msg_data<generator_t**>(&msg.data);

        auto request = request_block_generator_bulk_add{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.generators.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_del>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(string_t*);
        auto data = zmq_msg_data<string_t**>(&msg.data);

        auto request = request_block_generator_bulk_del{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_block_generator_start>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_generator_start{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_generator_stop{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_start>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(string_t*);
        auto data = zmq_msg_data<string_t**>(&msg.data);

        auto request = request_block_generator_bulk_start{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_stop>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(device_t*);
        auto data = zmq_msg_data<string_t**>(&msg.data);

        auto request = request_block_generator_bulk_stop{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg,
                              request_block_generator_result_list>(): {
        return (request_block_generator_result_list{});
    }
    case utils::variant_index<request_msg, request_block_generator_result>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_generator_result{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_block_generator_result_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_block_generator_result_del{std::move(id)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<reply_msg, reply_block_devices>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(device_t*);
        auto data = zmq_msg_data<device_t**>(&msg.data);

        auto reply = reply_block_devices{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.devices.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_block_files>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(file_t*);
        auto data = zmq_msg_data<file_t**>(&msg.data);

        auto reply = reply_block_files{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.files.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_block_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(generator_t*);
        auto data = zmq_msg_data<generator_t**>(&msg.data);

        auto reply = reply_block_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.generators.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_block_generator_results>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(generator_result_t*);
        auto data = zmq_msg_data<generator_result_t**>(&msg.data);

        auto reply = reply_block_generator_results{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.results.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{*(zmq_msg_data<typed_error*>(&msg.data))});
    }

    return (tl::make_unexpected(EINVAL));
}

int send_message(void* socket, serialized_msg&& msg)
{
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);
        return (errno);
    }

    return (0);
}

tl::expected<serialized_msg, int> recv_message(void* socket, int flags)
{
    serialized_msg msg;
    if (zmq_msg_init(&msg.type) == -1 || zmq_msg_init(&msg.data) == -1) {
        close(msg);
        return (tl::make_unexpected(ENOMEM));
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data, socket, flags) == -1) {
        close(msg);
        return (tl::make_unexpected(errno));
    }

    return (msg);
}

reply_error to_error(error_type type, int code, const std::string& value)
{
    auto err = reply_error{.info = typed_error{.type = type, .code = code}};
    value.copy(err.info.value, err_max_length - 1);
    err.info.value[std::min(value.length(), err_max_length)] = '\0';
    return err;
}

std::shared_ptr<BlockDevice> to_swagger(const device_t& p_device)
{
    auto device = std::make_shared<BlockDevice>();
    device->setId(p_device.get_id());
    device->setInfo(p_device.get_info());
    device->setPath(p_device.get_path());
    device->setSize(p_device.get_size());
    device->setUsable(p_device.is_usable());
    return device;
}

std::shared_ptr<BlockFile> to_swagger(const file_t& p_file)
{
    auto blkfile = std::make_shared<BlockFile>();
    blkfile->setId(p_file.get_id());
    blkfile->setPath(p_file.get_path());
    blkfile->setFileSize(p_file.get_size());
    blkfile->setInitPercentComplete(p_file.get_init_percent_complete());
    blkfile->setState(to_string(p_file.get_state()));
    return blkfile;
}

std::shared_ptr<BlockGenerator> to_swagger(const generator_t& p_gen)
{
    auto gen_config = std::make_shared<BlockGeneratorConfig>();
    gen_config->setPattern(to_string(p_gen.get_config().pattern));
    gen_config->setQueueDepth(p_gen.get_config().queue_depth);
    gen_config->setReadSize(p_gen.get_config().read_size);
    gen_config->setReadsPerSec(p_gen.get_config().reads_per_sec);
    gen_config->setWriteSize(p_gen.get_config().write_size);
    gen_config->setWritesPerSec(p_gen.get_config().writes_per_sec);

    auto gen = std::make_shared<BlockGenerator>();
    gen->setId(p_gen.get_id());
    gen->setResourceId(p_gen.get_resource_id());
    gen->setRunning(p_gen.is_running());
    gen->setConfig(gen_config);

    return gen;
}

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return (os.str());
}

std::shared_ptr<BlockGeneratorResult>
to_swagger(const generator_result_t& p_gen_result)
{
    auto gen_res = std::make_shared<BlockGeneratorResult>();
    gen_res->setId(p_gen_result.get_id());
    gen_res->setGeneratorId(p_gen_result.get_generator_id());
    gen_res->setActive(p_gen_result.is_active());
    gen_res->setTimestamp(
        to_rfc3339(p_gen_result.get_timestamp().time_since_epoch()));

    auto generate_gen_stat =
        [](const model::block_generator_statistics& gen_stat) {
            auto stat = std::make_shared<BlockGeneratorStats>();
            stat->setBytesActual(gen_stat.bytes_actual);
            stat->setBytesTarget(gen_stat.bytes_target);
            stat->setIoErrors(gen_stat.io_errors);
            stat->setOpsActual(gen_stat.ops_actual);
            stat->setOpsTarget(gen_stat.ops_target);
            stat->setLatency(gen_stat.latency.count());
            stat->setLatencyMin(gen_stat.latency_min.count());
            stat->setLatencyMax(gen_stat.latency_max.count());
            return stat;
        };

    gen_res->setRead(generate_gen_stat(p_gen_result.get_read_stats()));
    gen_res->setWrite(generate_gen_stat(p_gen_result.get_write_stats()));

    return gen_res;
}

model::file from_swagger(const BlockFile& p_file)
{
    model::file f;
    f.set_id(p_file.getId());
    f.set_path(p_file.getPath());
    f.set_size(p_file.getFileSize());
    f.set_init_percent_complete(p_file.getInitPercentComplete());
    return f;
}

model::block_generator from_swagger(const BlockGenerator& p_gen)
{
    model::block_generator gen;
    gen.set_id(p_gen.getId());
    gen.set_resource_id(p_gen.getResourceId());
    gen.set_running(p_gen.isRunning());
    gen.set_config((model::block_generator_config){
        p_gen.getConfig()->getQueueDepth(),
        p_gen.getConfig()->getReadsPerSec(),
        p_gen.getConfig()->getReadSize(),
        p_gen.getConfig()->getWritesPerSec(),
        p_gen.getConfig()->getWriteSize(),
        block_generation_pattern_from_string(p_gen.getConfig()->getPattern())});
    return gen;
}

request_block_file_bulk_add from_swagger(BulkCreateBlockFilesRequest& p_request)
{
    request_block_file_bulk_add request;
    for (const auto& item : p_request.getItems())
        request.files.emplace_back(std::make_unique<model::file>(from_swagger(*item)));
    return request;
}

request_block_file_bulk_del from_swagger(BulkDeleteBlockFilesRequest& p_request)
{
    request_block_file_bulk_del request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<string_t>(id));
    return request;
}

request_block_generator_bulk_add from_swagger(BulkCreateBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_add request;
    for (const auto& item : p_request.getItems())
        request.generators.emplace_back(std::make_unique<model::block_generator>(from_swagger(*item)));
    return request;
}

request_block_generator_bulk_del from_swagger(BulkDeleteBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_del request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<string_t>(id));
    return request;
}

request_block_generator_bulk_start
from_swagger(BulkStartBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_start request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<string_t>(id));
    return request;
}

request_block_generator_bulk_stop
from_swagger(BulkStopBlockGeneratorsRequest& p_request)
{
    request_block_generator_bulk_stop request;
    for (auto& id : p_request.getIds())
        request.ids.emplace_back(std::make_unique<string_t>(id));
    return request;
}

} // namespace openperf::block::api

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, BlockFile& file)
{
    file.setFileSize(j.at("file_size"));
    file.setPath(j.at("path"));

    if (j.find("id") != j.end()) file.setId(j.at("id"));

    if (j.find("init_percent_complete") != j.end())
        file.setInitPercentComplete(j.at("init_percent_complete"));

    if (j.find("state") != j.end()) file.setState(j.at("state"));
}

void from_json(const nlohmann::json& j, BlockGenerator& generator)
{
    generator.setResourceId(j.at("resource_id"));
    generator.setRunning(j.at("running"));

    if (j.find("id") != j.end()) { generator.setId(j.at("id")); }

    auto gc = BlockGeneratorConfig();
    gc.fromJson(const_cast<nlohmann::json&>(j.at("config")));
    generator.setConfig(std::make_shared<BlockGeneratorConfig>(gc));
}

void from_json(const nlohmann::json& j,
               BulkStartBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

void from_json(const nlohmann::json& j, BulkStopBlockGeneratorsRequest& request)
{
    request.fromJson(const_cast<nlohmann::json&>(j));
}

} // namespace swagger::v1::model