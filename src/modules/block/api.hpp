#ifndef _OP_BLOCK_API_HPP_
#define _OP_BLOCK_API_HPP_

#include <limits.h>
#include <string>
#include <variant>
#include <tl/expected.hpp>
#include "json.hpp"
#include "models/device.hpp"
#include "models/file.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"
#include "swagger/v1/model/BlockGenerator.h"
#include "swagger/v1/model/BlockGeneratorResult.h"
#include "swagger/v1/model/BulkStartBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopBlockGeneratorsRequest.h"
#include "swagger/v1/model/BlockFile.h"
#include "swagger/v1/model/BlockDevice.h"

namespace openperf::block::api {

using namespace swagger::v1::model;

const std::string endpoint = "inproc://openperf_block";

using device_t = model::device;
using device_ptr = std::unique_ptr<device_t>;

using file_t = model::file;
using file_ptr = std::unique_ptr<file_t>;

using generator_t = model::block_generator;
using generator_ptr = std::unique_ptr<generator_t>;

using generator_result_t = model::block_generator_result;
using generator_result_ptr = std::unique_ptr<generator_result_t>;

using string_t = std::string;
using string_ptr = std::unique_ptr<string_t>;

static constexpr size_t err_max_length = 256;
enum class error_type { NONE = 0, NOT_FOUND, ZMQ_ERROR, CUSTOM_ERROR };
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
                                 request_block_file_list,
                                 request_block_file,
                                 request_block_file_add,
                                 request_block_file_del,
                                 request_block_generator_list,
                                 request_block_generator,
                                 request_block_generator_add,
                                 request_block_generator_del,
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

struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg);
tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg);

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

reply_error to_error(error_type type, int code = 0, std::string value = "");
const char* to_string(const api::typed_error&);
std::shared_ptr<BlockDevice> to_swagger(const model::device&);
std::shared_ptr<BlockFile> to_swagger(const model::file&);
std::shared_ptr<BlockGenerator> to_swagger(const model::block_generator&);
std::shared_ptr<BlockGeneratorResult> to_swagger(const model::block_generator_result&);
model::file from_swagger(const BlockFile&);
model::block_generator from_swagger(const BlockGenerator&);
request_block_generator_bulk_start from_swagger(BulkStartBlockGeneratorsRequest&);
request_block_generator_bulk_stop from_swagger(BulkStopBlockGeneratorsRequest&);

extern const std::string endpoint;

} // namespace openperf::block::api
#endif /* _OP_BLOCK_API_HPP_ */
