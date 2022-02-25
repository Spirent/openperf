#include "memory/generator/worker_api.hpp"
#include "framework/message/serialized_message.hpp"
#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"

namespace openperf::memory::generator::worker {

serialized_msg serialize(command_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const start_msg& msg) -> int {
                     return (message::push(serialized, msg.endpoint)
                             || message::push(serialized, msg.result));
                 },
                 [&](const stop_msg& msg) {
                     return (message::push(serialized, msg.endpoint));
                 },
                 [&](const term_msg& msg) { return (0); },
                 [&](const update_msg& msg) -> int {
                     return (message::push(serialized, msg.load.read)
                             || message::push(serialized, msg.load.write));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<command_msg, int> deserialize(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<command_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<command_msg, start_msg>(): {
        return (
            start_msg{message::pop_string(msg), message::pop<result*>(msg)});
    }
    case utils::variant_index<command_msg, stop_msg>(): {
        return (stop_msg{message::pop_string(msg)});
    }
    case utils::variant_index<command_msg, term_msg>(): {
        return (tl::make_unexpected(ETERM));
    }
    case utils::variant_index<command_msg, update_msg>(): {
        return (update_msg{
            .load = {.read = message::pop_vector<ops_per_sec>(msg),
                     .write = message::pop_vector<ops_per_sec>(msg)}});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

} // namespace openperf::memory::generator::worker
