#include <zmq.h>
#include <sstream>
#include <iomanip>
#include <netdb.h>

#include "api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::cpu::api {

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

void copy_string(std::string_view str, char* ch_arr, size_t max_length)
{
    str.copy(ch_arr, max_length - 1);
    ch_arr[std::min(str.length(), max_length - 1)] = '\0';
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
                    [&](const request_cpu_generator_list&) {
                        return (zmq_msg_init(&serialized.data));
                    },
                    [&](const request_cpu_generator& blkgenerator) {
                        return (zmq_msg_init(&serialized.data,
                                             blkgenerator.id.data(),
                                             blkgenerator.id.length()));
                    },
                    [&](const request_cpu_generator_add& blkgenerator) {
                        return (
                            zmq_msg_init(&serialized.data,
                                         std::addressof(blkgenerator.source),
                                         sizeof(blkgenerator.source)));
                    },
                    [&](const request_cpu_generator_del& blkgenerator) {
                        return (zmq_msg_init(&serialized.data,
                                             blkgenerator.id.data(),
                                             blkgenerator.id.length()));
                    },
                    [&](const request_cpu_generator_start& blkgenerator) {
                        return (zmq_msg_init(&serialized.data,
                                             blkgenerator.id.data(),
                                             blkgenerator.id.length()));
                    },
                    [&](const request_cpu_generator_stop& blkgenerator) {
                        return (zmq_msg_init(&serialized.data,
                                             blkgenerator.id.data(),
                                             blkgenerator.id.length()));
                    },
                    [&](const request_cpu_generator_bulk_start&
                            blkgenerator) {
                        auto scalar =
                            sizeof(decltype(blkgenerator.ids)::value_type);
                        return (zmq_msg_init(&serialized.data,
                                             blkgenerator.ids.data(),
                                             scalar * blkgenerator.ids.size()));
                    },
                    [&](const request_cpu_generator_bulk_stop& blkgenerator) {
                        auto scalar =
                            sizeof(decltype(blkgenerator.ids)::value_type);
                        return (zmq_msg_init(&serialized.data,
                                             blkgenerator.ids.data(),
                                             scalar * blkgenerator.ids.size()));
                    },
                    [&](const request_cpu_generator_result_list&) {
                        return (zmq_msg_init(&serialized.data));
                    },
                    [&](const request_cpu_generator_result& result) {
                        return (zmq_msg_init(&serialized.data,
                                             result.id.data(),
                                             result.id.length()));
                    },
                    [&](const request_cpu_generator_result_del& result) {
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
                    [&](const reply_cpu_generators& blkgenerators) {
                        /*
                         * ZMQ wants the length in bytes, so we have to scale
                         * the length of the vector up to match.
                         */
                        auto scalar = sizeof(
                            decltype(blkgenerators.generators)::value_type);
                        return (zmq_msg_init(
                            &serialized.data,
                            blkgenerators.generators.data(),
                            scalar * blkgenerators.generators.size()));
                    },
                    [&](const reply_cpu_generator_bulk_start& reply) {
                        /*
                         * ZMQ wants the length in bytes, so we have to scale
                         * the length of the vector up to match.
                         */
                        auto scalar =
                            sizeof(decltype(reply.failed)::value_type);
                        return (zmq_msg_init(&serialized.data,
                                             reply.failed.data(),
                                             scalar * reply.failed.size()));
                    },
                    [&](const reply_cpu_generator_results& results) {
                        /*
                         * ZMQ wants the length in bytes, so we have to scale
                         * the length of the vector up to match.
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
    case utils::variant_index<request_msg, request_cpu_generator_list>(): {
        return (request_cpu_generator_list{});
    }
    case utils::variant_index<request_msg, request_cpu_generator_add>(): {
        return (
            request_cpu_generator_add{*zmq_msg_data<generator*>(&msg.data)});
    }
    case utils::variant_index<request_msg, request_cpu_generator>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_cpu_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_cpu_generator_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_cpu_generator_del{std::move(id)});
    }
    case utils::variant_index<request_msg, request_cpu_generator_start>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_cpu_generator_start{std::move(id)});
    }
    case utils::variant_index<request_msg, request_cpu_generator_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_cpu_generator_stop{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_start>(): {
        auto data =
            zmq_msg_data<request_cpu_generator_bulk_start::container*>(
                &msg.data);
        std::vector<request_cpu_generator_bulk_start::container>
            blkgenerators(
                data,
                data
                    + zmq_msg_size<
                          request_cpu_generator_bulk_start::container>(
                          &msg.data));
        return (request_cpu_generator_bulk_start{std::move(blkgenerators)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_stop>(): {
        auto data = zmq_msg_data<request_cpu_generator_bulk_stop::container*>(
            &msg.data);
        std::vector<request_cpu_generator_bulk_stop::container> blkgenerators(
            data,
            data
                + zmq_msg_size<request_cpu_generator_bulk_stop::container>(
                      &msg.data));
        return (request_cpu_generator_bulk_stop{std::move(blkgenerators)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_list>(): {
        return (request_cpu_generator_result_list{});
    }
    case utils::variant_index<request_msg, request_cpu_generator_result>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_cpu_generator_result{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_cpu_generator_result_del{std::move(id)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<reply_msg, reply_cpu_generators>(): {
        auto data = zmq_msg_data<generator*>(&msg.data);
        std::vector<generator> blkgenerators(
            data, data + zmq_msg_size<generator>(&msg.data));
        return (reply_cpu_generators{std::move(blkgenerators)});
    }
    case utils::variant_index<reply_msg, reply_cpu_generator_bulk_start>(): {
        auto data = zmq_msg_data<failed_generator*>(&msg.data);
        std::vector<failed_generator> failed_generators(
            data, data + zmq_msg_size<failed_generator>(&msg.data));
        return (reply_cpu_generator_bulk_start{std::move(failed_generators)});
    }
    case utils::variant_index<reply_msg, reply_cpu_generator_results>(): {
        auto data = zmq_msg_data<generator_result*>(&msg.data);
        std::vector<generator_result> results(
            data, data + zmq_msg_size<generator_result>(&msg.data));
        return (reply_cpu_generator_results{std::move(results)});
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

} // namespace openperf::cpu::api