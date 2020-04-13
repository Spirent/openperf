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
static auto zmq_msg_init(zmq_msg_t* msg, std::unique_ptr<T> value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T**>(zmq_msg_data(msg));
    *ptr = value.release();
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

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg,
                         std::vector<std::unique_ptr<T>>& values)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*) * values.size());
        error != 0) {
        return (error);
    }

    auto cursor = reinterpret_cast<T**>(zmq_msg_data(msg));
    std::transform(std::begin(values), std::end(values), cursor, [](auto& ptr) {
        return (ptr.release());
    });
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

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
                utils::overloaded_visitor(
                    [&](const request_cpu_generator_list&) {
                        return (zmq_msg_init(&serialized.data));
                    },
                    [&](const request_cpu_generator& cpu_generator) {
                        return (zmq_msg_init(&serialized.data,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](request_cpu_generator_add& cpu_generator) {
                        return (
                            zmq_msg_init(&serialized.data,
                                         std::move(cpu_generator.source)));
                    },
                    [&](const request_cpu_generator_del& cpu_generator) {
                        return (zmq_msg_init(&serialized.data,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_start& cpu_generator) {
                        return (zmq_msg_init(&serialized.data,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_stop& cpu_generator) {
                        return (zmq_msg_init(&serialized.data,
                                             cpu_generator.id.data(),
                                             cpu_generator.id.length()));
                    },
                    [&](const request_cpu_generator_bulk_start&
                            cpu_generator) {
                        auto scalar =
                            sizeof(decltype(cpu_generator.ids)::value_type);
                        return (zmq_msg_init(&serialized.data,
                                             cpu_generator.ids.data(),
                                             scalar * cpu_generator.ids.size()));
                    },
                    [&](const request_cpu_generator_bulk_stop& cpu_generator) {
                        auto scalar =
                            sizeof(decltype(cpu_generator.ids)::value_type);
                        return (zmq_msg_init(&serialized.data,
                                             cpu_generator.ids.data(),
                                             scalar * cpu_generator.ids.size()));
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

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
                utils::overloaded_visitor(
                    [&](reply_cpu_generators& reply) {
                        return (zmq_msg_init(&serialized.data, reply.generators));
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
                    [&](const reply_cpu_generator_results& reply) {
                        /*
                         * ZMQ wants the length in bytes, so we have to scale
                         * the length of the vector up to match.
                         */
                        auto scalar =
                            sizeof(decltype(reply.results)::value_type);
                        return (zmq_msg_init(&serialized.data,
                                             reply.results.data(),
                                             scalar * reply.results.size()));
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
        auto request = request_cpu_generator_add{};
        request.source.reset(*zmq_msg_data<cpu_generator_t**>(&msg.data));
        return (request);
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
            cpu_generators(
                data,
                data
                    + zmq_msg_size<
                          request_cpu_generator_bulk_start::container>(
                          &msg.data));
        return (request_cpu_generator_bulk_start{std::move(cpu_generators)});
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_stop>(): {
        auto data = zmq_msg_data<request_cpu_generator_bulk_stop::container*>(
            &msg.data);
        std::vector<request_cpu_generator_bulk_stop::container> cpu_generators(
            data,
            data
                + zmq_msg_size<request_cpu_generator_bulk_stop::container>(
                      &msg.data));
        return (request_cpu_generator_bulk_stop{std::move(cpu_generators)});
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
        auto size = zmq_msg_size(&msg.data) / sizeof(cpu_generator_t*);
        auto data = zmq_msg_data<cpu_generator_t**>(&msg.data);

        auto reply = reply_cpu_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.generators.emplace_back(ptr);
        });
        return (reply);
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

model::cpu_generator from_swagger(const CpuGenerator& p_gen)
{
    model::cpu_generator gen;
    gen.set_id(p_gen.getId());
    gen.set_running(p_gen.isRunning());
    auto cores_config = p_gen.getConfig()->getCores();
    model::cpu_generator_config cpu_config;
    for (auto p_conf : cores_config) {
        auto targets = p_conf->getTargets();
        model::cpu_generator_core_config core_conf {
            .utilization = p_conf->getUtilization()
        };
        for (auto p_target : targets) {
            core_conf.targets.push_back(model::cpu_generator_target_config {
                .instruction_set = cpu_instruction_set_from_string(p_target->getInstructionSet()),
                .data_size = static_cast<uint>(p_target->getDataSize()),
                .operation = cpu_operation_from_string(p_target->getOperation()),
                .weight = static_cast<uint>(p_target->getWeight())
            });
        }
        cpu_config.cores.push_back(core_conf);
    }
    gen.set_config(cpu_config);
    return gen;
}

std::shared_ptr<CpuGenerator> to_swagger(const model::cpu_generator& p_gen)
{
    auto gen = std::make_shared<CpuGenerator>();
    gen->setId(p_gen.get_id());
    gen->setRunning(p_gen.is_running());
    auto cores_config = p_gen.get_config().cores;
    auto cpu_config = std::make_shared<CpuGeneratorConfig>();
    for (auto p_conf : cores_config) {
        auto core_conf = std::make_shared<CpuGeneratorCoreConfig>();
        core_conf->setUtilization(p_conf.utilization);
        for (auto p_target : p_conf.targets) {
            auto target = std::make_shared<CpuGeneratorCoreConfig_targets>();
            target->setDataSize(p_target.data_size);
            target->setInstructionSet(to_string(p_target.instruction_set));
            target->setOperation(to_string(p_target.operation));
            target->setWeight(p_target.weight);
            core_conf->getTargets().push_back(target);
        }
        cpu_config->getCores().push_back(core_conf);
    }
    gen->setConfig(cpu_config);
    return gen;
}


} // namespace openperf::cpu::api