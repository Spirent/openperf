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

template <typename T> static T zmq_msg_data(const zmq_msg_t* msg)
{
    return (reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

template <typename T> static size_t zmq_msg_size(const zmq_msg_t* msg)
{
    return (zmq_msg_size(const_cast<zmq_msg_t*>(msg)) / sizeof(T));
}

static void close(serialized_msg& msg)
{
    zmq_close(&msg.type);
    zmq_close(&msg.data);
}

void copy_string(std::string_view str, char* ch_arr, size_t max_length) {
    str.copy(ch_arr, max_length - 1);
    ch_arr[std::min(str.length(), max_length - 1)] = '\0';
}

const static std::unordered_map<std::string, model::block_generation_pattern> block_generation_patterns = {
    {"random", model::block_generation_pattern::RANDOM},
    {"sequential", model::block_generation_pattern::SEQUENTIAL},
    {"reverse", model::block_generation_pattern::REVERSE},
};

const static std::unordered_map<model::block_generation_pattern, std::string> block_generation_pattern_strings = {
    {model::block_generation_pattern::RANDOM, "random"},
    {model::block_generation_pattern::SEQUENTIAL, "sequential"},
    {model::block_generation_pattern::REVERSE, "reverse"},
};

model::block_generation_pattern block_generation_pattern_from_string(const std::string& value) {
    if (block_generation_patterns.count(value))
        return block_generation_patterns.at(value);
    throw std::runtime_error("Pattern \"" + value + "\" is unknown");
}

std::string to_string(const model::block_generation_pattern& pattern) {
    if (block_generation_pattern_strings.count(pattern))
        return block_generation_pattern_strings.at(pattern);
    return "unknown";
}

const static std::unordered_map<std::string, model::file_state> block_file_states = {
    {"none", model::file_state::NONE},
    {"init", model::file_state::INIT},
    {"ready", model::file_state::READY},
};

const static std::unordered_map<model::file_state, std::string> block_file_state_strings = {
    {model::file_state::NONE, "none"},
    {model::file_state::INIT, "init"},
    {model::file_state::READY, "ready"},
};

model::file_state block_file_state_from_string(const std::string& value) {
    if (block_file_states.count(value))
        return block_file_states.at(value);
    throw std::runtime_error("Pattern \"" + value + "\" is unknown");
}

std::string to_string(const model::file_state& pattern) {
    if (block_file_state_strings.count(pattern))
        return block_file_state_strings.at(pattern);
    return "unknown";
}

const char* to_string(const api::typed_error& error)
{
    switch (error.type) {
    case api::error_type::NOT_FOUND:
        return ("");
    case api::error_type::EAI_ERROR:
        return (gai_strerror(error.code));
    case api::error_type::ZMQ_ERROR:
        return (zmq_strerror(error.code));
    case api::error_type::CUSTOM_ERROR:
        return error.value;
    default:
        return ("unknown error type");
    }
}

serialized_msg serialize_request(const request_msg& msg)
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
                [&](const request_block_file_add& blkfile) {
                    return (zmq_msg_init(&serialized.data,
                                          std::addressof(blkfile.source),
                                          sizeof(blkfile.source)));
                },
                [&](const request_block_file_del& blkfile) {
                    return (zmq_msg_init(&serialized.data,
                                          blkfile.id.data(),
                                          blkfile.id.length()));
                },
                [&](const request_block_generator_list&) {
                    return (zmq_msg_init(&serialized.data));
                },
                [&](const request_block_generator& blkgenerator) {
                    return (zmq_msg_init(&serialized.data,
                                          blkgenerator.id.data(),
                                          blkgenerator.id.length()));
                },
                [&](const request_block_generator_add& blkgenerator) {
                    return (zmq_msg_init(&serialized.data,
                                          std::addressof(blkgenerator.source),
                                          sizeof(blkgenerator.source)));
                },
                [&](const request_block_generator_del& blkgenerator) {
                    return (zmq_msg_init(&serialized.data,
                                          blkgenerator.id.data(),
                                          blkgenerator.id.length()));
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
                [&](const request_block_generator_bulk_start& blkgenerator) {
                    auto scalar =
                         sizeof(decltype(blkgenerator.ids)::value_type);
                    return (zmq_msg_init(&serialized.data,
                                          blkgenerator.ids.data(),
                                          scalar * blkgenerator.ids.size()));
                },
                [&](const request_block_generator_bulk_stop& blkgenerator) {
                    auto scalar =
                         sizeof(decltype(blkgenerator.ids)::value_type);
                    return (zmq_msg_init(&serialized.data,
                                          blkgenerator.ids.data(),
                                          scalar * blkgenerator.ids.size()));
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

serialized_msg serialize_reply(const reply_msg& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const reply_block_devices& blkdevices) {
                     /*
                      * ZMQ wants the length in bytes, so we have to scale the
                      * length of the vector up to match.
                      */
                     auto scalar =
                         sizeof(decltype(blkdevices.devices)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          blkdevices.devices.data(),
                                          scalar * blkdevices.devices.size()));
                 },
                 [&](const reply_block_files& blkfiles) {
                     /*
                      * ZMQ wants the length in bytes, so we have to scale the
                      * length of the vector up to match.
                      */
                     auto scalar =
                         sizeof(decltype(blkfiles.files)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          blkfiles.files.data(),
                                          scalar * blkfiles.files.size()));
                 },
                 [&](const reply_block_generators& blkgenerators) {
                     /*
                      * ZMQ wants the length in bytes, so we have to scale the
                      * length of the vector up to match.
                      */
                     auto scalar =
                         sizeof(decltype(blkgenerators.generators)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          blkgenerators.generators.data(),
                                          scalar * blkgenerators.generators.size()));
                 },
                 [&](const reply_block_generator_bulk_start& reply) {
                     /*
                      * ZMQ wants the length in bytes, so we have to scale the
                      * length of the vector up to match.
                      */
                     auto scalar =
                         sizeof(decltype(reply.failed)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          reply.failed.data(),
                                          scalar * reply.failed.size()));
                 },
                 [&](const reply_block_generator_results& results) {
                     /*
                      * ZMQ wants the length in bytes, so we have to scale the
                      * length of the vector up to match.
                      */
                     auto scalar =
                         sizeof(decltype(results.results)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          results.results.data(),
                                          scalar * results.results.size()));
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
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_device{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_file_list>(): {
        return (request_block_file_list{});
    }
    case utils::variant_index<request_msg, request_block_file_add>(): {
        return (request_block_file_add{*zmq_msg_data<file*>(&msg.data)});
    }
    case utils::variant_index<request_msg, request_block_file>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_file{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_file_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_file_del{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_list>(): {
        return (request_block_generator_list{});
    }
    case utils::variant_index<request_msg, request_block_generator_add>(): {
        return (request_block_generator_add{*zmq_msg_data<generator*>(&msg.data)});
    }
    case utils::variant_index<request_msg, request_block_generator>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator_del{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_start>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator_start{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator_stop{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_bulk_start>(): {
        auto data = zmq_msg_data<request_block_generator_bulk_start::container*>(&msg.data);
        std::vector<request_block_generator_bulk_start::container> blkgenerators(
            data, data + zmq_msg_size<request_block_generator_bulk_start::container>(&msg.data));
        return (request_block_generator_bulk_start{std::move(blkgenerators)});
    }
    case utils::variant_index<request_msg, request_block_generator_bulk_stop>(): {
        auto data = zmq_msg_data<request_block_generator_bulk_stop::container*>(&msg.data);
        std::vector<request_block_generator_bulk_stop::container> blkgenerators(
            data, data + zmq_msg_size<request_block_generator_bulk_stop::container>(&msg.data));
        return (request_block_generator_bulk_stop{std::move(blkgenerators)});
    }
    case utils::variant_index<request_msg, request_block_generator_result_list>(): {
        return (request_block_generator_result_list{});
    }
    case utils::variant_index<request_msg, request_block_generator_result>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_result_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
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
        auto data = zmq_msg_data<device*>(&msg.data);
        std::vector<device> blkdevices(
            data, data + zmq_msg_size<device>(&msg.data));
        return (reply_block_devices{std::move(blkdevices)});
    }
    case utils::variant_index<reply_msg, reply_block_files>(): {
        auto data = zmq_msg_data<file*>(&msg.data);
        std::vector<file> blkfiles(
            data, data + zmq_msg_size<file>(&msg.data));
        return (reply_block_files{std::move(blkfiles)});
    }
    case utils::variant_index<reply_msg, reply_block_generators>(): {
        auto data = zmq_msg_data<generator*>(&msg.data);
        std::vector<generator> blkgenerators(
            data, data + zmq_msg_size<generator>(&msg.data));
        return (reply_block_generators{std::move(blkgenerators)});
    }
    case utils::variant_index<reply_msg, reply_block_generator_bulk_start>(): {
        auto data = zmq_msg_data<failed_generator*>(&msg.data);
        std::vector<failed_generator> failed_generators(
            data, data + zmq_msg_size<failed_generator>(&msg.data));
        return (reply_block_generator_bulk_start{std::move(failed_generators)});
    }
    case utils::variant_index<reply_msg, reply_block_generator_results>(): {
        auto data = zmq_msg_data<generator_result*>(&msg.data);
        std::vector<generator_result> results(
            data, data + zmq_msg_size<generator_result>(&msg.data));
        return (reply_block_generator_results{std::move(results)});
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

reply_error to_error(error_type type, int code, std::string value)
{
    auto err = reply_error{.info = typed_error{.type = type, .code = code}};
    value.copy(err.info.value, err_max_length - 1);
    err.info.value[std::min(value.length(), err_max_length)] = '\0';
    return err;
}

std::shared_ptr<BlockDevice> to_swagger(const device& p_device)
{
    auto device = std::make_shared<BlockDevice>();
    device->setId(p_device.id);
    device->setInfo(p_device.info);
    device->setPath(p_device.path);
    device->setSize(p_device.size);
    device->setUsable(p_device.usable);
    return device;
}

std::shared_ptr<BlockFile> to_swagger(const file& p_file)
{
    auto blkfile = std::make_shared<BlockFile>();
    blkfile->setId(p_file.id);
    blkfile->setPath(p_file.path);
    blkfile->setFileSize(p_file.size);
    blkfile->setInitPercentComplete(p_file.init_percent_complete);
    blkfile->setState(to_string(p_file.state));
    return blkfile;
}

std::shared_ptr<BlockGenerator> to_swagger(const generator& p_gen)
{
    auto gen_config = std::make_shared<BlockGeneratorConfig>();
    gen_config->setPattern(to_string(p_gen.config.pattern));
    gen_config->setQueueDepth(p_gen.config.queue_depth);
    gen_config->setReadSize(p_gen.config.read_size);
    gen_config->setReadsPerSec(p_gen.config.reads_per_sec);
    gen_config->setWriteSize(p_gen.config.write_size);
    gen_config->setWritesPerSec(p_gen.config.writes_per_sec);

    auto gen = std::make_shared<BlockGenerator>();
    gen->setId(p_gen.id);
    gen->setResourceId(p_gen.resource_id);
    gen->setRunning(p_gen.running);
    gen->setConfig(gen_config);

    return gen;
}

std::shared_ptr<BulkStartBlockGeneratorsResponse> to_swagger(const reply_block_generator_bulk_start& request)
{
    auto resp = std::make_shared<BulkStartBlockGeneratorsResponse>();
    for (auto gen : request.failed) {
        if (gen.err.type == error_type::NONE) {
            resp->getSucceeded().push_back(gen.id);
        } else {
            auto failed = std::make_shared<BulkStartBlockGeneratorsResponse_failed>();
            failed->setId(gen.id);
            failed->setError(to_string(gen.err));
            resp->getFailed().push_back(failed);
        }

    }
    return resp;
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

std::shared_ptr<BlockGeneratorResult> to_swagger(const generator_result& p_gen_result)
{
    auto gen_res = std::make_shared<BlockGeneratorResult>();
    gen_res->setId(p_gen_result.id);
    gen_res->setActive(p_gen_result.active);
    gen_res->setTimestamp(to_rfc3339(p_gen_result.timestamp.time_since_epoch()));

    auto generate_gen_stat = [](const generator_stats& gen_stat){
        auto stat = std::make_shared<BlockGeneratorStats>();
        stat->setBytesActual(gen_stat.bytes_actual);
        stat->setBytesTarget(gen_stat.bytes_target);
        stat->setIoErrors(gen_stat.io_errors);
        stat->setOpsActual(gen_stat.ops_actual);
        stat->setOpsTarget(gen_stat.ops_target);
        stat->setLatency(gen_stat.latency);
        stat->setLatencyMin(gen_stat.latency_min);
        stat->setLatencyMax(gen_stat.latency_max);
        return stat;
    };

    gen_res->setRead(generate_gen_stat(p_gen_result.read_stats));
    gen_res->setWrite(generate_gen_stat(p_gen_result.write_stats));

    return gen_res;
}

file from_swagger(const BlockFile& p_file)
{
    auto f = file{};
    f.size = p_file.getFileSize();
    f.init_percent_complete = p_file.getInitPercentComplete();
    f.state = block_file_state_from_string(p_file.getState());
    copy_string(p_file.getId(), f.id, id_max_length);
    copy_string(p_file.getPath(), f.path, path_max_length);
    return f;
}

generator from_swagger(const BlockGenerator& p_gen)
{
    auto gen = generator{};
    copy_string(p_gen.getId(), gen.id, id_max_length);
    copy_string(p_gen.getResourceId(), gen.resource_id, id_max_length);
    gen.running = p_gen.isRunning();
    gen.config.pattern = block_generation_pattern_from_string(p_gen.getConfig()->getPattern());
    gen.config.queue_depth = p_gen.getConfig()->getQueueDepth();
    gen.config.read_size = p_gen.getConfig()->getReadSize();
    gen.config.reads_per_sec = p_gen.getConfig()->getReadsPerSec();
    gen.config.write_size = p_gen.getConfig()->getWriteSize();
    gen.config.writes_per_sec = p_gen.getConfig()->getWritesPerSec();
    return gen;
}

request_block_generator_bulk_start from_swagger(BulkStartBlockGeneratorsRequest& request)
{
    request_block_generator_bulk_start req;
    for (auto id : request.getIds()) {
        request_block_generator_bulk_start::container c;
        copy_string(id, c.id, id_max_length);
        req.ids.push_back(c);
    }
    return req;
}

request_block_generator_bulk_stop from_swagger(BulkStopBlockGeneratorsRequest& request)
{
    request_block_generator_bulk_stop req;
    for (auto id : request.getIds()) {
        request_block_generator_bulk_stop::container c;
        copy_string(id, c.id, id_max_length);
        req.ids.push_back(c);
    }
    return req;
}

device to_api_model(const model::device& p_device)
{
    auto dev = device{};
    dev.size = p_device.get_size();
    dev.usable = p_device.is_usable();
    copy_string(p_device.get_id(), dev.id, id_max_length);
    copy_string(p_device.get_path(), dev.path, path_max_length);
    copy_string(p_device.get_info(), dev.info, device_info_max_length);
    return dev;
}

file to_api_model(const model::file& p_file)
{
    auto f = file{};
    f.size = p_file.get_size();
    f.init_percent_complete = p_file.get_init_percent_complete();
    f.state = p_file.get_state();
    copy_string(p_file.get_id(), f.id, id_max_length);
    copy_string(p_file.get_path(), f.path, path_max_length);
    return f;
}

generator to_api_model(const model::block_generator& p_gen)
{
    auto gen = generator{};
    copy_string(p_gen.get_id(), gen.id, id_max_length);
    copy_string(p_gen.get_resource_id(), gen.resource_id, id_max_length);
    gen.running = p_gen.is_running();

    gen.config.pattern = p_gen.get_config().pattern;
    gen.config.queue_depth = p_gen.get_config().queue_depth;
    gen.config.read_size = p_gen.get_config().read_size;
    gen.config.reads_per_sec = p_gen.get_config().reads_per_sec;
    gen.config.write_size = p_gen.get_config().write_size;
    gen.config.writes_per_sec = p_gen.get_config().writes_per_sec;

    return gen;
}

generator_result to_api_model(const model::block_generator_result& p_gen_result)
{
    generator_result gen_res;
    copy_string(p_gen_result.get_id(), gen_res.id, id_max_length);
    gen_res.active = p_gen_result.is_active();
    gen_res.timestamp = p_gen_result.get_timestamp();

    auto generate_gen_stat = [](const model::block_generator_statistics& gen_stat){
        generator_stats stat;
        stat.bytes_actual = gen_stat.bytes_actual;
        stat.bytes_target = gen_stat.bytes_target;
        stat.io_errors = gen_stat.io_errors;
        stat.ops_actual = gen_stat.ops_actual;
        stat.ops_target = gen_stat.ops_target;
        stat.latency = gen_stat.latency;
        stat.latency_min = gen_stat.latency_min;
        stat.latency_max = gen_stat.latency_max;
        return stat;
    };


    gen_res.read_stats = generate_gen_stat(p_gen_result.get_read_stats());
    gen_res.write_stats = generate_gen_stat(p_gen_result.get_write_stats());

    return gen_res;
}

std::shared_ptr<model::file> from_api_model(const file& p_file)
{
    auto f = std::make_shared<model::file>();
    f->set_id(p_file.id);
    f->set_path(p_file.path);
    f->set_size(p_file.size);
    f->set_init_percent_complete(p_file.init_percent_complete);
    return f;
}

std::shared_ptr<model::block_generator> from_api_model(const generator& p_gen)
{
    auto gen = std::make_shared<model::block_generator>();
    gen->set_id(p_gen.id);
    gen->set_resource_id(p_gen.resource_id);
    gen->set_running(p_gen.running);
    gen->set_config((model::block_generator_config){
        p_gen.config.queue_depth,
        p_gen.config.reads_per_sec,
        p_gen.config.read_size,
        p_gen.config.writes_per_sec,
        p_gen.config.write_size,
        p_gen.config.pattern
    });
    return gen;
}

}