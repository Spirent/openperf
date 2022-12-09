#ifndef _OP_CPU_GENERATOR_WORKER_API_HPP_
#define _OP_CPU_GENERATOR_WORKER_API_HPP_

#include <future>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "tl/expected.hpp"

#include "core/op_socket.h"
#include "generator/v2/worker_client.hpp"

namespace openperf::message {
struct serialized_message;
}

namespace openperf::cpu::generator {

struct core_config;
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

struct update_msg
{
    std::vector<double> values;
};

using command_msg = std::variant<start_msg, stop_msg, term_msg, update_msg>;

class client_impl
    : public openperf::generator::worker::
          client<client_impl, result, std::vector<double>>
{
public:
    using client::client;

    int
    send_start(void* socket, std::string_view syncpoint, result* result) const;
    int send_stop(void* socket, std::string_view syncpoint) const;
    int send_terminate(void* socket) const;
    int send_update(void* socket, std::vector<double>&& load) const;
};

using client = client_impl;

int do_work(void* context,
            uint8_t coordinator_id,
            uint8_t core_id,
            const core_config& config,
            const char* endpoint,
            std::promise<void> promise);

using serialized_msg = openperf::message::serialized_message;

serialized_msg serialize_command(command_msg&& msg);
tl::expected<command_msg, int> deserialize_command(serialized_msg&& msg);

} // namespace worker
} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_WORKER_API_HPP_ */
