#ifndef _OP_BLOCK_API_HPP_
#define _OP_BLOCK_API_HPP_

#include <climits>
#include <string>
#include <variant>

#include <tl/expected.hpp>
#include <zmq.h>

#include "models/device.hpp"
#include "models/file.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::message {
struct serialized_message;
};

namespace openperf::block::api {

const std::string endpoint = "inproc://openperf_block";

using openperf::message::serialized_message;
using device_ptr = std::unique_ptr<model::device>;
using file_ptr = std::unique_ptr<model::file>;
using generator_ptr = std::unique_ptr<model::block_generator>;
using generator_result_ptr = std::unique_ptr<model::block_generator_result>;
using string_ptr = std::unique_ptr<std::string>;

static constexpr size_t err_max_length = 256;
enum class error_type : uint8_t {
    NONE = 0,
    NOT_FOUND,
    ZMQ_ERROR,
    CUSTOM_ERROR
};

struct typed_error
{
    error_type type = error_type::NONE;
    int code = 0;
    char value[err_max_length];
};

/* zmq api request models */

struct request_block_device_list
{};

struct request_block_device
{
    std::string id;
};

struct request_block_device_init
{
    std::string id;
};

struct request_block_file_list
{};

struct request_block_file
{
    std::string id;
};

struct request_block_file_add
{
    file_ptr source;
};

struct request_block_file_del
{
    std::string id;
};

struct request_block_file_bulk_add
{
    std::vector<file_ptr> files;
};

struct request_block_file_bulk_del
{
    std::vector<string_ptr> ids;
};

struct request_block_generator_list
{};

struct request_block_generator
{
    std::string id;
};

struct request_block_generator_add
{
    generator_ptr source;
};

struct request_block_generator_del
{
    std::string id;
};

struct request_block_generator_bulk_add
{
    std::vector<generator_ptr> generators;
};

struct request_block_generator_bulk_del
{
    std::vector<string_ptr> ids;
};

struct request_block_generator_start
{
    std::string id;
};

struct request_block_generator_stop
{
    std::string id;
};

struct request_block_generator_bulk_start
{
    std::vector<string_ptr> ids;
};

struct request_block_generator_bulk_stop
{
    std::vector<string_ptr> ids;
};

struct request_block_generator_result_list
{};

struct request_block_generator_result
{
    std::string id;
};

struct request_block_generator_result_del
{
    std::string id;
};

/* zmq api reply models */

struct reply_block_devices
{
    std::vector<device_ptr> devices;
};

struct reply_block_files
{
    std::vector<file_ptr> files;
};

struct reply_block_generators
{
    std::vector<generator_ptr> generators;
};

struct reply_block_generator_results
{
    std::vector<generator_result_ptr> results;
};

struct reply_ok
{};

struct reply_error
{
    typed_error info;
};

using request_msg = std::variant<request_block_device_list,
                                 request_block_device,
                                 request_block_device_init,
                                 request_block_file_list,
                                 request_block_file,
                                 request_block_file_add,
                                 request_block_file_del,
                                 request_block_file_bulk_add,
                                 request_block_file_bulk_del,
                                 request_block_generator_list,
                                 request_block_generator,
                                 request_block_generator_add,
                                 request_block_generator_del,
                                 request_block_generator_bulk_add,
                                 request_block_generator_bulk_del,
                                 request_block_generator_start,
                                 request_block_generator_stop,
                                 request_block_generator_bulk_start,
                                 request_block_generator_bulk_stop,
                                 request_block_generator_result_list,
                                 request_block_generator_result,
                                 request_block_generator_result_del>;

using reply_msg = std::variant<reply_block_devices,
                               reply_block_files,
                               reply_block_generators,
                               reply_block_generator_results,
                               reply_ok,
                               reply_error>;

serialized_message serialize_request(request_msg&&);
serialized_message serialize_reply(reply_msg&&);

tl::expected<request_msg, int> deserialize_request(serialized_message&&);
tl::expected<reply_msg, int> deserialize_reply(serialized_message&&);

reply_error
to_error(error_type type, int code = 0, const std::string& value = "");
const char* to_string(const api::typed_error&);

} // namespace openperf::block::api

#endif /* _OP_BLOCK_API_HPP_ */
