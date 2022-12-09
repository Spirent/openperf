#ifndef _OP_MEMORY_GENERATOR_WORKER_API_HPP_
#define _OP_MEMORY_GENERATOR_WORKER_API_HPP_

#include <future>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "tl/expected.hpp"

#include "core/op_socket.h"
#include "generator/v2/worker_client.hpp"
#include "memory/generator/config.hpp"

namespace openperf::message {
struct serialized_message;
}

namespace openperf::memory::generator {

class result;

namespace worker {

struct start_msg
{
    std::string endpoint;
    result* result;
};

struct stop_msg
{
    std::string endpoint;
};

struct term_msg
{};

struct io_load
{
    std::vector<ops_per_sec> read;
    std::vector<ops_per_sec> write;
};

struct update_msg
{
    io_load load;
};

using command_msg = std::variant<start_msg, stop_msg, term_msg, update_msg>;

class client_impl
    : public openperf::generator::worker::client<client_impl, result, io_load>
{
public:
    using client::client;

    int
    send_start(void* socket, std::string_view syncpoint, result* result) const;
    int send_stop(void* socket, std::string_view syncpoint) const;
    int send_terminate(void* socket) const;
    int send_update(void* socket, io_load&& load) const;
};

using client = client_impl;

struct io_config
{
    mutable std::byte* buffer;
    struct
    {
        size_t min;
        size_t max;
    } index_range;
    size_t io_size;
    ops_per_sec io_rate;
    enum api::pattern_type io_pattern;
};

int do_reads(void* context,
             uint8_t coordinator_id,
             uint8_t thread_id,
             const char* endpoint,
             io_config config,
             std::promise<void> promise);

int do_writes(void* context,
              uint8_t coordinator_id,
              uint8_t thread_id,
              const char* endpoint,
              io_config config,
              std::promise<void> promise);

using serialized_msg = openperf::message::serialized_message;

serialized_msg serialize(command_msg&& msg);
tl::expected<command_msg, int> deserialize(serialized_msg&& msg);

} // namespace worker
} // namespace openperf::memory::generator

#endif /* _OP_MEMORY_GENERATOR_WORKER_API_HPP_ */
