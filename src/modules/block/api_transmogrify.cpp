#include <zmq.h>
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

void copy_string(const std::string& str, char* ch_arr) {
    std::copy(str.begin(), str.end(), ch_arr);
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
                    printf("Size %lu\n", scalar);
                    return (zmq_msg_init(&serialized.data,
                                          blkgenerator.ids.data(),
                                          scalar * blkgenerator.ids.size()));
                },
                [&](const request_block_generator_bulk_stop& blkgenerator) {
                    auto scalar =
                         sizeof(decltype(blkgenerator.ids)::value_type);
                    printf("Size %lu\n", scalar);
                    return (zmq_msg_init(&serialized.data,
                                          blkgenerator.ids.data(),
                                          scalar * blkgenerator.ids.size()));
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
        return (request_block_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_bulk_start>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_block_generator_bulk_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data),
                       zmq_msg_size(&msg.data));
        return (request_block_generator{std::move(id)});
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

reply_error to_error(error_type type, int value)
{
    return (reply_error{.info = typed_error{.type = type, .value = value}});
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
    auto device = std::make_shared<BlockFile>();
    device->setId(p_file.id);
    device->setPath(p_file.path);
    device->setFileSize(p_file.size);
    device->setInitPercentComplete(p_file.init_percent_complete);
    device->setState(p_file.state);
    return device;
}

file from_swagger(const BlockFile& p_file)
{
    auto f = file{};
    f.size = p_file.getFileSize();
    f.init_percent_complete = p_file.getInitPercentComplete();
    copy_string(p_file.getId(), f.id);
    copy_string(p_file.getPath(), f.path);
    copy_string(p_file.getState(), f.state);
    return f;
}

std::shared_ptr<model::block_generator>
from_swagger(const BlockGenerator& p_generator)
{
    auto generator = std::make_shared<model::block_generator>();
    generator->set_id(p_generator.getId());
    generator->set_resource_id(p_generator.getResourceId());
    generator->set_running(p_generator.isRunning());

    model::block_generator_config config;
    config.pattern = block_generation_pattern_from_string(p_generator.getConfig()->getPattern());
    config.queue_depth = p_generator.getConfig()->getQueueDepth();
    config.read_size = p_generator.getConfig()->getReadSize();
    config.reads_per_sec = p_generator.getConfig()->getReadsPerSec();
    config.write_size = p_generator.getConfig()->getWriteSize();
    config.writes_per_sec = p_generator.getConfig()->getWritesPerSec();

    generator->set_config(config);

    return generator;
}

device to_api_model(const model::device& p_device)
{
    auto dev = device{};
    dev.size = p_device.get_size();
    dev.usable = p_device.is_usable();
    copy_string(p_device.get_id(), dev.id);
    copy_string(p_device.get_path(), dev.path);
    copy_string(p_device.get_info(), dev.info);
    return dev;
}

file to_api_model(const model::file& p_file)
{
    auto f = file{};
    f.size = p_file.get_size();
    f.init_percent_complete = p_file.get_init_percent_complete();
    copy_string(p_file.get_id(), f.id);
    copy_string(p_file.get_path(), f.path);
    copy_string(p_file.get_state(), f.state);
    return f;
}

std::shared_ptr<model::file> from_api_model(const file& p_file)
{
    auto f = std::make_shared<model::file>();
    f->set_id(p_file.id);
    f->set_path(p_file.path);
    f->set_state(p_file.state);
    f->set_size(p_file.size);
    f->set_init_percent_complete(p_file.init_percent_complete);
    return f;
}

}