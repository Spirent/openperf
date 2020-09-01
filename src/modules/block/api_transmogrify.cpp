#include "api.hpp"

#include <zmq.h>

#include "framework/utils/overloaded_visitor.hpp"
#include "framework/utils/variant_index.hpp"
#include "framework/message/serialized_message.hpp"

namespace openperf::block::api {

message::serialized_message serialize_request(request_msg&& msg)
{
    message::serialized_message serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_block_device_list&) { return (0); },
                 [&](const request_block_device& blkdevice) {
                     return message::push(serialized, blkdevice.id);
                 },
                 [&](const request_block_device_init& blkdevice) {
                     return message::push(serialized, blkdevice.id);
                 },
                 [&](const request_block_file_list&) { return (0); },
                 [&](const request_block_file& blkfile) {
                     return message::push(serialized, blkfile.id);
                 },
                 [&](request_block_file_add& blkfile) {
                     return message::push(serialized,
                                          std::move(blkfile.source));
                 },
                 [&](const request_block_file_del& blkfile) {
                     return message::push(serialized, blkfile.id);
                 },
                 [&](request_block_file_bulk_add& request) {
                     return message::push(serialized, request.files);
                 },
                 [&](request_block_file_bulk_del& request) {
                     return message::push(
                         serialized,
                         std::make_unique<request_block_file_bulk_del>(
                             std::move(request)));
                 },
                 [&](const request_block_generator_list&) { return (0); },
                 [&](const request_block_generator& blkgenerator) {
                     return message::push(serialized, blkgenerator.id);
                 },
                 [&](request_block_generator_add& blkgenerator) {
                     return message::push(serialized,
                                          std::move(blkgenerator.source));
                 },
                 [&](const request_block_generator_del& blkgenerator) {
                     return message::push(serialized, blkgenerator.id);
                 },
                 [&](request_block_generator_bulk_add& request) {
                     return message::push(serialized, request.generators);
                 },
                 [&](request_block_generator_bulk_del& request) {
                     return message::push(
                         serialized,
                         std::make_unique<request_block_generator_bulk_del>(
                             std::move(request)));
                 },
                 [&](request_block_generator_start& start) {
                     message::push(serialized, start.id);
                     return message::push(
                         serialized,
                         std::make_unique<dynamic::configuration>(
                             std::move(start.dynamic_results)));
                 },
                 [&](const request_block_generator_stop& blkgenerator) {
                     return message::push(serialized, blkgenerator.id);
                 },
                 [&](request_block_generator_bulk_start& request) {
                     return message::push(
                         serialized,
                         std::make_unique<request_block_generator_bulk_start>(
                             std::move(request)));
                 },
                 [&](request_block_generator_bulk_stop& request) {
                     return message::push(
                         serialized,
                         std::make_unique<request_block_generator_bulk_stop>(
                             std::move(request)));
                 },
                 [&](const request_block_generator_result_list&) {
                     return (0);
                 },
                 [&](const request_block_generator_result& result) {
                     return message::push(serialized, result.id);
                 },
                 [&](const request_block_generator_result_del& result) {
                     return message::push(serialized, result.id);
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

message::serialized_message serialize_reply(reply_msg&& msg)
{
    message::serialized_message serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](reply_block_devices& reply) {
                               return message::push(serialized, reply.devices);
                           },
                           [&](reply_block_files& reply) {
                               return message::push(serialized, reply.files);
                           },
                           [&](reply_block_generators& reply) {
                               return message::push(serialized,
                                                    reply.generators);
                           },
                           [&](reply_block_generator_results& reply) {
                               return message::push(serialized, reply.results);
                           },
                           [&](const reply_ok&) { return (0); },
                           [&](const reply_error& error) {
                               return message::push(serialized, error.info);
                           }),
                       msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int>
deserialize_request(message::serialized_message&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<request_msg, request_block_device_list>(): {
        return request_block_device_list{};
    }
    case utils::variant_index<request_msg, request_block_device>(): {
        return request_block_device{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_block_device_init>(): {
        return request_block_device_init{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_block_file_list>(): {
        return request_block_file_list{};
    }
    case utils::variant_index<request_msg, request_block_file_add>(): {
        auto request = request_block_file_add{};
        request.source.reset(message::pop<model::file*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request_block_file>(): {
        return request_block_file{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_block_file_del>(): {
        return request_block_file_del{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_block_file_bulk_add>(): {
        return request_block_file_bulk_add{
            message::pop_unique_vector<model::file>(msg)};
    }
    case utils::variant_index<request_msg, request_block_file_bulk_del>(): {
        return *std::unique_ptr<request_block_file_bulk_del>(
            message::pop<request_block_file_bulk_del*>(msg));
    }
    case utils::variant_index<request_msg, request_block_generator_list>(): {
        return request_block_generator_list{};
    }
    case utils::variant_index<request_msg, request_block_generator_add>(): {
        auto request = request_block_generator_add{};
        request.source.reset(message::pop<model::block_generator*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request_block_generator>(): {
        return request_block_generator{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg, request_block_generator_del>(): {
        return request_block_generator_del{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_add>(): {
        return request_block_generator_bulk_add{
            message::pop_unique_vector<model::block_generator>(msg)};
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_del>(): {
        return *std::unique_ptr<request_block_generator_bulk_del>(
            message::pop<request_block_generator_bulk_del*>(msg));
    }
    case utils::variant_index<request_msg, request_block_generator_start>(): {
        request_block_generator_start request{};
        request.id = message::pop_string(msg);
        request.dynamic_results = *std::unique_ptr<dynamic::configuration>(
            message::pop<dynamic::configuration*>(msg));
        return request;
    }
    case utils::variant_index<request_msg, request_block_generator_stop>(): {
        return request_block_generator_stop{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_start>(): {
        return *std::unique_ptr<request_block_generator_bulk_start>(
            message::pop<request_block_generator_bulk_start*>(msg));
    }
    case utils::variant_index<request_msg,
                              request_block_generator_bulk_stop>(): {
        return *std::unique_ptr<request_block_generator_bulk_stop>(
            message::pop<request_block_generator_bulk_stop*>(msg));
    }
    case utils::variant_index<request_msg,
                              request_block_generator_result_list>(): {
        return request_block_generator_result_list{};
    }
    case utils::variant_index<request_msg, request_block_generator_result>(): {
        return request_block_generator_result{message::pop_string(msg)};
    }
    case utils::variant_index<request_msg,
                              request_block_generator_result_del>(): {
        return request_block_generator_result_del{message::pop_string(msg)};
    }
    }

    return tl::make_unexpected(EINVAL);
}

tl::expected<reply_msg, int>
deserialize_reply(message::serialized_message&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_block_devices>(): {
        return reply_block_devices{
            message::pop_unique_vector<model::device>(msg)};
    }
    case utils::variant_index<reply_msg, reply_block_files>(): {
        return reply_block_files{message::pop_unique_vector<model::file>(msg)};
    }
    case utils::variant_index<reply_msg, reply_block_generators>(): {
        return reply_block_generators{
            message::pop_unique_vector<model::block_generator>(msg)};
    }
    case utils::variant_index<reply_msg, reply_block_generator_results>(): {
        return reply_block_generator_results{
            message::pop_unique_vector<model::block_generator_result>(msg)};
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return reply_ok{};
    case utils::variant_index<reply_msg, reply_error>():
        return reply_error{message::pop<typed_error>(msg)};
    }

    return tl::make_unexpected(EINVAL);
}

reply_error to_error(error_type type, int code, const std::string& value)
{
    auto err = reply_error{.info = typed_error{.type = type, .code = code}};
    value.copy(err.info.value, err_max_length - 1);
    err.info.value[std::min(value.length(), err_max_length)] = '\0';
    return err;
}

const char* to_string(const api::typed_error& error)
{
    switch (error.type) {
    case api::error_type::NOT_FOUND:
        return "";
    case api::error_type::ZMQ_ERROR:
        return zmq_strerror(error.code);
    case api::error_type::CUSTOM_ERROR:
        return error.value;
    default:
        return "unknown error type";
    }
}

} // namespace openperf::block::api
