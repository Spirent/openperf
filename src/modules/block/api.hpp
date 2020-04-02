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
#include "swagger/v1/model/BulkStartBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartBlockGeneratorsResponse.h"
#include "swagger/v1/model/BlockGeneratorResult.h"
#include "swagger/v1/model/BlockFile.h"
#include "swagger/v1/model/BlockDevice.h"

namespace openperf::block::api {

const std::string endpoint = "inproc://openperf_block";
using namespace swagger::v1::model;
using time_point = std::chrono::time_point<timesync::chrono::realtime>;

/**
 * List of supported server requests.
 */
enum class request_type {
    NONE = 0,
    LIST_GENERATOR_RESULTS,
    GET_GENERATOR_RESULT,
    DELETE_GENERATOR_RESULT
};

static constexpr size_t id_max_length = 64;
static constexpr size_t path_max_length = PATH_MAX;
static constexpr size_t device_info_max_length = 1024;
static constexpr size_t file_state_max_length = 64;

/* zmq api objects models */

struct device
{
    char id[id_max_length];
    char path[path_max_length];
    int64_t size;
    char info[device_info_max_length];
    bool usable;
};

struct file
{
    char id[id_max_length];
    char path[path_max_length];
    int64_t size;
    char state[file_state_max_length];
    int32_t init_percent_complete;
};

struct generator
{
    char id[id_max_length];
    char resource_id[id_max_length];
    bool running;
    struct {
        int32_t queue_depth;
        int32_t reads_per_sec;
        int32_t read_size;
        int32_t writes_per_sec;
        int32_t write_size;
        model::block_generation_pattern pattern;
    } config;
};

struct generator_stats
{
    int64_t ops_target;
    int64_t ops_actual;
    int64_t bytes_target;
    int64_t bytes_actual;
    int64_t io_errors;
    int64_t latency;
    int64_t latency_min;
    int64_t latency_max;
};

struct generator_result
{
    char id[id_max_length];
    bool active;
    time_point timestamp;
    generator_stats read_stats;
    generator_stats write_stats;
};

enum class error_type { NONE = 0, NOT_FOUND, EAI_ERROR, ZMQ_ERROR };

struct typed_error
{
    error_type type = error_type::NONE;
    int value = 0;
};

/* zmq api request models */

struct request_block_device_list{};

struct request_block_device
{
    std::string id;
};
struct request_block_file_list{};

struct request_block_file
{
    std::string id;
};

struct request_block_file_add
{
    file source;
};

struct request_block_file_del
{
    std::string id;
};

struct request_block_generator_list{};

struct request_block_generator
{
    std::string id;
};

struct request_block_generator_add
{
    generator source;
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
    struct container {
        char id[id_max_length];
    };
    std::vector<container> ids;
};

struct request_block_generator_bulk_stop
{
    struct container {
        char id[id_max_length];
    };
    std::vector<container> ids;
};

struct request_block_generator_result_list{};

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
    std::vector<device> devices;
};

struct reply_block_files
{
    std::vector<file> files;
};

struct reply_block_generators
{
    std::vector<generator> generators;
};

struct failed_generator {
    char id[id_max_length];
    typed_error err;
};

struct reply_block_generator_bulk_start
{
    std::vector<failed_generator> failed;
};

struct reply_block_generator_results
{
    std::vector<generator_result> results;
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
                               reply_block_generator_bulk_start,
                               reply_block_generator_results,
                               reply_ok,
                               reply_error>;

struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

enum class reply_code { NONE = 0, OK, NO_DEVICE, NO_FILE, NO_GENERATOR, BAD_INPUT, ERROR };

std::string to_string(request_type type);
std::string to_string(reply_code code);
serialized_msg serialize_request(const request_msg& request);
serialized_msg serialize_reply(const reply_msg& reply);

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg);
tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg);

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

reply_error to_error(error_type type, int value = 0);
const char* to_string(const api::typed_error&);
std::shared_ptr<BlockDevice> to_swagger(const device&);
std::shared_ptr<BlockFile> to_swagger(const file&);
std::shared_ptr<BlockGenerator> to_swagger(const generator&);
std::shared_ptr<BlockGeneratorResult> to_swagger(const generator_result&);
std::shared_ptr<BulkStartBlockGeneratorsResponse> to_swagger(const reply_block_generator_bulk_start&);
file from_swagger(const BlockFile&);
generator from_swagger(const BlockGenerator&);
request_block_generator_bulk_start from_swagger(BulkStartBlockGeneratorsRequest&);
request_block_generator_bulk_stop from_swagger(BulkStopBlockGeneratorsRequest&);
device to_api_model(const model::device&);
file to_api_model(const model::file&);
generator to_api_model(const model::block_generator&);
generator_result to_api_model(const model::block_generator_result&);
std::shared_ptr<model::file> from_api_model(const file&);
std::shared_ptr<model::block_generator> from_api_model(const generator&);

inline std::string json_error(int code, const char* message)
{
    return (nlohmann::json({{"code", code}, {"message", message}}).dump());
}

extern const std::string endpoint;

} // namespace openperf::block::api
#endif /* _OP_BLOCK_API_HPP_ */
