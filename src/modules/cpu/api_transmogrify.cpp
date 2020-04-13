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
static auto zmq_msg_add(std::vector<zmq_msg_t>& msg, const T& value)
{
    msg.push_back(zmq_msg_t{});
    return zmq_msg_init(&msg.back(), value);
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

template <typename T>
static auto zmq_msg_add(std::vector<zmq_msg_t>& msg, const T* buffer, size_t length)
{s
    msg.push_back(zmq_msg_t{});
    return zmq_msg_init(&msg.back(), buffer, length);
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
    zmq_close(&msg.data_size);
    for (auto& m : msg.data) {
        zmq_close(&m);
    }
}

int send_message(void* socket, serialized_msg&& msg)
{
    assert(msg.data.size() > 0);

    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data_size, socket, ZMQ_SNDMORE) == -1) {
        close(msg);
        return (errno);
    }

    for (auto& m : msg.data) {
        if (zmq_msg_send(&m, socket, (&m == &msg.data.back()) ? 0 : ZMQ_SNDMORE) == -1) {
            close(msg);
            return (errno);
        }
    }

    return (0);
}

tl::expected<serialized_msg, int> recv_message(void* socket, int flags)
{
    serialized_msg msg;
    if (zmq_msg_init(&msg.type) == -1 || zmq_msg_init(&msg.data_size) == -1) {
        close(msg);
        return (tl::make_unexpected(ENOMEM));
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data_size, socket, flags) == -1) {
        close(msg);
        return (tl::make_unexpected(errno));
    }
    for (size_t i = 0; i < *zmq_msg_data<size_t*>(&msg.data_size); ++i) {
        msg.data.push_back(zmq_msg_t{});
        if (zmq_msg_init(&msg.data.back()) == -1 || zmq_msg_recv(&msg.data.back(), socket, flags) == -1) {
            close(msg);
            return (tl::make_unexpected(errno));
        }
    }

    return (msg);
}

int serialize_generator(const generator& cpu_generator, std::vector<zmq_msg_t>& data)
{
    assert(cpu_generator.config_size == cpu_generator.cores_config.size());
    if (auto r = zmq_msg_add(data, std::addressof(cpu_generator), sizeof(cpu_generator)))
        return r;
    for (auto& cc : cpu_generator.cores_config) {
        assert(cc.targets.size() == cc.targets_size);
        if (auto r = zmq_msg_add(data, std::addressof(cc), sizeof(cc)))
            return r;
        for (auto& tc : cc.targets) {
            if (auto r = zmq_msg_add(data, std::addressof(tc), sizeof(tc)))
                return r;
        }
    }
    return 0;
}

serialized_msg serialize_request(const request_msg& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
                utils::overloaded_visitor(
                    [&](const request_cpu_generator_list&) {
                        return 0;
                    },
                    [&](const request_cpu_generator& cpu_generator) {
                        return (zmq_msg_init(&serialized.data.back(),
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_add& cpu_generator) {
                        return serialize_generator(cpu_generator.source, serialized.data);
                    },
                    [&](const request_cpu_generator_del& cpu_generator) {
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_start& cpu_generator) {
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_stop& cpu_generator) {
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_bulk_start&
                            cpu_generator) {
                        auto scalar =
                            sizeof(decltype(cpu_generator.ids)::value_type);
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             cpu_generator.ids.data(),
                                             scalar * cpu_generator.ids.size()));
                    },
                    [&](const request_cpu_generator_bulk_stop& cpu_generator) {
                        auto scalar =
                            sizeof(decltype(cpu_generator.ids)::value_type);
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             cpu_generator.ids.data(),
                                             scalar * cpu_generator.ids.size()));
                    },
                    [&](const request_cpu_generator_result_list&) {
                        return 0;
                    },
                    [&](const request_cpu_generator_result& result) {
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             result.id.data(),
                                             result.id.length()));
                    },
                    [&](const request_cpu_generator_result_del& result) {
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             result.id.data(),
                                             result.id.length()));
                    }),
                msg) || zmq_msg_init(&serialized.data_size, serialized.data.size()));
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
                    [&](const reply_cpu_generators& cpu_generators) {
                        /*
                         * ZMQ wants the length in bytes, so we have to scale
                         * the length of the vector up to match.
                         */
                        assert(cpu_generators.generators_size == cpu_generators.generators.size());
                        if (auto r = zmq_msg_add(serialized.data, std::addressof(cpu_generators), sizeof(cpu_generators)))
                                return r;
                        for (auto& cpu_generator : cpu_generators.generators) {
                            serialize_generator(cpu_generator, serialized.data);
                        }
                        return 0;
                    },
                    [&](const reply_cpu_generator_bulk_start& reply) {
                        /*
                         * ZMQ wants the length in bytes, so we have to scale
                         * the length of the vector up to match.
                         */
                        auto scalar =
                            sizeof(decltype(reply.failed)::value_type);
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
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
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m,
                                             results.results.data(),
                                             scalar * results.results.size()));
                    },
                    [&](const reply_ok&) {
                        return 0;
                    },
                    [&](const reply_error& error) {
                        zmq_msg_t m;
                        serialized.data.push_back(m);
                        return (zmq_msg_init(&m, error.info));
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
        auto request = *zmq_msg_data<generator*>(&msg.data.front());
        std::vector<generator_core_config> cores_config(request.config_size, generator_core_config{});
        size_t n = 0;
        for (size_t i = 0; i < request.config_size; ++i) {
            auto& gcc = *zmq_msg_data<generator_core_config*>(&msg.data.at(++n));
            std::vector<generator_target_config> gtc;
            for (size_t j = 0; j < request.cores_config.back().targets_size; ++j) {
                gtc.push_back(*zmq_msg_data<generator_target_config*>(&msg.data.at(++n)));
            }
            gcc.targets = gtc;
            cores_config[i] = gcc;
        }
        request.cores_config = cores_config;

        return (
            request_cpu_generator_add{request});
    }
    case utils::variant_index<request_msg, request_cpu_generator>(): {
        std::string id(zmq_msg_data<char*>(&msg.data.front()), zmq_msg_size(&msg.data.front()));
        return (request_cpu_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_cpu_generator_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data.front()), zmq_msg_size(&msg.data.front()));
        return (request_cpu_generator_del{std::move(id)});
    }
    case utils::variant_index<request_msg, request_cpu_generator_start>(): {
        std::string id(zmq_msg_data<char*>(&msg.data.front()), zmq_msg_size(&msg.data.front()));
        return (request_cpu_generator_start{std::move(id)});
    }
    case utils::variant_index<request_msg, request_cpu_generator_stop>(): {
        std::string id(zmq_msg_data<char*>(&msg.data.front()), zmq_msg_size(&msg.data.front()));
        return (request_cpu_generator_stop{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_start>(): {
        auto data =
            zmq_msg_data<request_cpu_generator_bulk_start::container*>(
                &msg.data.front());
        std::vector<request_cpu_generator_bulk_start::container>
            cpu_generators(
                data,
                data
                    + zmq_msg_size<
                          request_cpu_generator_bulk_start::container>(
                          &msg.data.front()));
        return (request_cpu_generator_bulk_start{std::move(cpu_generators)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_stop>(): {
        auto data = zmq_msg_data<request_cpu_generator_bulk_stop::container*>(
            &msg.data.front());
        std::vector<request_cpu_generator_bulk_stop::container> cpu_generators(
            data,
            data
                + zmq_msg_size<request_cpu_generator_bulk_stop::container>(
                      &msg.data.front()));
        return (request_cpu_generator_bulk_stop{std::move(cpu_generators)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_list>(): {
        return (request_cpu_generator_result_list{});
    }
    case utils::variant_index<request_msg, request_cpu_generator_result>(): {
        std::string id(zmq_msg_data<char*>(&msg.data.front()), zmq_msg_size(&msg.data.front()));
        return (request_cpu_generator_result{std::move(id)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_result_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data.front()), zmq_msg_size(&msg.data.front()));
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
        auto data = zmq_msg_data<generator*>(&msg.data.front());
        std::vector<generator> cpu_generators(
            data, data + zmq_msg_size<generator>(&msg.data.front()));
        return (reply_cpu_generators{std::move(cpu_generators)});
    }
    case utils::variant_index<reply_msg, reply_cpu_generator_bulk_start>(): {
        auto data = zmq_msg_data<failed_generator*>(&msg.data.front());
        std::vector<failed_generator> failed_generators(
            data, data + zmq_msg_size<failed_generator>(&msg.data.front()));
        return (reply_cpu_generator_bulk_start{std::move(failed_generators)});
    }
    case utils::variant_index<reply_msg, reply_cpu_generator_results>(): {
        auto data = zmq_msg_data<generator_result*>(&msg.data.front());
        std::vector<generator_result> results(
            data, data + zmq_msg_size<generator_result>(&msg.data.front()));
        return (reply_cpu_generator_results{std::move(results)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{*(zmq_msg_data<typed_error*>(&msg.data.front()))});
    }

    return (tl::make_unexpected(EINVAL));
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

const static std::unordered_map<std::string, model::cpu_instruction_set>
    cpu_instruction_sets = {
        {"scalar", model::cpu_instruction_set::SCALAR},
};

const static std::unordered_map<model::cpu_instruction_set, std::string>
    cpu_instruction_set_strings = {
        {model::cpu_instruction_set::SCALAR, "scalar"},
};

model::cpu_instruction_set
cpu_instruction_set_from_string(const std::string& value)
{
    if (cpu_instruction_sets.count(value))
        return cpu_instruction_sets.at(value);
    throw std::runtime_error("Instruction set \"" + value + "\" is unknown");
}

std::string to_string(const model::cpu_instruction_set& pattern)
{
    if (cpu_instruction_set_strings.count(pattern))
        return cpu_instruction_set_strings.at(pattern);
    return "unknown";
}

const static std::unordered_map<std::string, model::cpu_operation>
    cpu_operations = {
        {"int", model::cpu_operation::INT},
        {"float", model::cpu_operation::FLOAT},
};

const static std::unordered_map<model::cpu_operation, std::string>
    cpu_operation_strings = {
        {model::cpu_operation::INT, "int"},
        {model::cpu_operation::FLOAT, "float"},
};

model::cpu_operation
cpu_operation_from_string(const std::string& value)
{
    if (cpu_operations.count(value))
        return cpu_operations.at(value);
    throw std::runtime_error("Instruction set \"" + value + "\" is unknown");
}

std::string to_string(const model::cpu_operation& pattern)
{
    if (cpu_operation_strings.count(pattern))
        return cpu_operation_strings.at(pattern);
    return "unknown";
}

reply_error to_error(error_type type, int code, std::string value)
{
    auto err = reply_error{.info = typed_error{.type = type, .code = code}};
    value.copy(err.info.value, err_max_length - 1);
    err.info.value[std::min(value.length(), err_max_length)] = '\0';
    return err;
}

generator from_swagger(const CpuGenerator& p_gen)
{
    auto gen = generator{};
    copy_string(p_gen.getId(), gen.id, id_max_length);
    gen.running = p_gen.isRunning();
    auto cores_config = p_gen.getConfig()->getCores();
    gen.config_size = cores_config.size();
    for (auto p_conf : cores_config) {
        auto targets = p_conf->getTargets();
        generator_core_config conf {
            .targets_size = targets.size()
        };
        for (auto p_target : targets) {
            conf.targets.push_back(generator_target_config {
                .instruction_set = cpu_instruction_set_from_string(p_target->getInstructionSet()),
                .data_size = static_cast<uint>(p_target->getDataSize()),
                .operation = cpu_operation_from_string(p_target->getOperation()),
                .weight = static_cast<uint>(p_target->getWeight())
            });
        }
        gen.cores_config.push_back(conf);
    }
    return gen;
}

} // namespace openperf::cpu::api