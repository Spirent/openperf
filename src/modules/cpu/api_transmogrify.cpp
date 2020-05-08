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
                    [&](request_cpu_generator_bulk_start& request) {
                        return (zmq_msg_init(&serialized.data, std::move(request.ids)));
                    },
                    [&](request_cpu_generator_bulk_stop& request) {
                        return (zmq_msg_init(&serialized.data, std::move(request.ids)));
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
                    },
                    [&](const request_cpu_info&) {
                        return (zmq_msg_init(&serialized.data));
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
                    [&](reply_cpu_generator_results& reply) {
                        return (zmq_msg_init(&serialized.data, reply.results));
                    },
                    [&](reply_cpu_info& reply) {
                        return (zmq_msg_init(&serialized.data, std::move(reply.info)));
                    },
                    [&](const reply_ok&) {
                        return (zmq_msg_init(&serialized.data, 0));
                    },
                    [&](reply_error& error) {
                        return (zmq_msg_init(&serialized.data, std::move(error.info)));
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
        auto request = request_cpu_generator_bulk_start{};
        request.ids.reset(*zmq_msg_data<std::vector<std::string>**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg,
                              request_cpu_generator_bulk_stop>(): {
        auto request = request_cpu_generator_bulk_stop{};
        request.ids.reset(*zmq_msg_data<std::vector<std::string>**>(&msg.data));
        return (request);
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
    case utils::variant_index<request_msg,
                              request_cpu_info>(): {
        return (request_cpu_info{});
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
    case utils::variant_index<reply_msg, reply_cpu_generator_results>(): {
       auto size = zmq_msg_size(&msg.data) / sizeof(cpu_generator_result_t*);
        auto data = zmq_msg_data<cpu_generator_result_t**>(&msg.data);

        auto reply = reply_cpu_generator_results{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.results.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_cpu_info>(): {
        auto reply = reply_cpu_info{};
        reply.info.reset(*zmq_msg_data<cpu_info_t**>(&msg.data));
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        auto reply = reply_error{};
        reply.info.reset(*zmq_msg_data<typed_error**>(&msg.data));
        return (reply);
    }

    return (tl::make_unexpected(EINVAL));
}

std::string to_string(const api::typed_error& error)
{
    switch (error.type) {
    case api::error_type::NOT_FOUND:
        return ("");
    case api::error_type::ZMQ_ERROR:
        return (zmq_strerror(error.code));
    case api::error_type::CUSTOM_ERROR:
        return error.value;
    default:
        return ("unknown error type");
    }
}

cpu::instruction_set cpu_instruction_set_from_string(const std::string& value)
{
    const static std::unordered_map<std::string, cpu::instruction_set>
        cpu_instruction_sets = {
            {"scalar", cpu::instruction_set::SCALAR},
    };

    if (cpu_instruction_sets.count(value))
        return cpu_instruction_sets.at(value);
    throw std::runtime_error(
        "Instruction set \"" + value + "\" is unknown");
}

std::string to_string(const cpu::instruction_set& pattern)
{
    const static std::unordered_map<cpu::instruction_set, std::string>
        cpu_instruction_set_strings = {
            {cpu::instruction_set::SCALAR, "scalar"},
    };

    if (cpu_instruction_set_strings.count(pattern))
        return cpu_instruction_set_strings.at(pattern);
    return "unknown";
}

cpu::data_type cpu_data_type_from_string(const std::string& value)
{
    const static std::unordered_map<std::string, cpu::data_type>
        cpu_data_types = {
            {"int32", cpu::data_type::INT32},
            {"int64", cpu::data_type::INT64},
            {"float32", cpu::data_type::FLOAT32},
            {"float64", cpu::data_type::FLOAT64},
    };

    if (cpu_data_types.count(value))
        return cpu_data_types.at(value);
    throw std::runtime_error("Instruction set \"" + value + "\" is unknown");
}

std::string to_string(const cpu::data_type& pattern)
{
    const static std::unordered_map<cpu::data_type, std::string>
        cpu_data_type_strings = {
            {cpu::data_type::INT32, "int32"},
            {cpu::data_type::INT64, "int64"},
            {cpu::data_type::FLOAT32, "float32"},
            {cpu::data_type::FLOAT64, "float64"},
    };

    if (cpu_data_type_strings.count(pattern))
        return cpu_data_type_strings.at(pattern);
    return "unknown";
}

reply_error to_error(error_type type, int code, const std::string& value)
{
    auto err = reply_error{.info = std::make_unique<typed_error>((typed_error){.type = type, .code = code, .value = value})};
    return err;
}

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return (os.str());
}


model::generator from_swagger(const CpuGenerator& p_gen)
{
    model::generator_config config;
    for (auto p_conf : p_gen.getConfig()->getCores()) {
        model::generator_core_config core_conf {
            .utilization = p_conf->getUtilization()
        };

        for (auto p_target : p_conf->getTargets()) {
            core_conf.targets.push_back(model::generator_target_config {
                .instruction_set = cpu_instruction_set_from_string(p_target->getInstructionSet()),
                .data_type = cpu_data_type_from_string(p_target->getDataType()),
                .weight = static_cast<uint>(p_target->getWeight())
            });
        }

        config.cores.push_back(core_conf);
    }

    model::generator gen;
    gen.id(p_gen.getId());
    gen.running(p_gen.isRunning());
    gen.config(config);
    return gen;
}

std::shared_ptr<CpuGenerator> to_swagger(const model::generator& p_gen)
{
    auto cpu_config = std::make_shared<CpuGeneratorConfig>();
    for (auto p_conf : p_gen.config().cores) {
        auto core_conf = std::make_shared<CpuGeneratorCoreConfig>();
        for (auto p_target : p_conf.targets) {
            auto target = std::make_shared<CpuGeneratorCoreConfig_targets>();
            target->setDataType(to_string(p_target.data_type));
            target->setInstructionSet(to_string(p_target.instruction_set));
            target->setWeight(p_target.weight);
            core_conf->getTargets().push_back(target);
        }

        cpu_config->getCores().push_back(core_conf);
        core_conf->setUtilization(p_conf.utilization);
    }

    auto gen = std::make_shared<CpuGenerator>();
    gen->setId(p_gen.id());
    gen->setRunning(p_gen.running());
    gen->setConfig(cpu_config);
    return gen;
}

std::shared_ptr<CpuGeneratorResult> to_swagger(const model::generator_result& p_result)
{
    auto cpu_stats = std::make_shared<CpuGeneratorStats>();
    for (auto p_stats : p_result.stats().cores) {
        auto core_stats = std::make_shared<CpuGeneratorCoreStats>();
        for (auto p_target : p_stats.targets) {
            auto target = std::make_shared<CpuGeneratorTargetStats>();
            target->setCycles(p_target.cycles);
            core_stats->getTargets().push_back(target);
        }
        cpu_stats->getCores().push_back(core_stats);

        core_stats->setAvailable(p_stats.available);
        core_stats->setError(p_stats.error);
        core_stats->setSteal(p_stats.steal);
        core_stats->setSystem(p_stats.system);
        core_stats->setUser(p_stats.user);
        core_stats->setUtilization(p_stats.utilization);
    }

    auto gen = std::make_shared<CpuGeneratorResult>();
    gen->setId(p_result.id());
    gen->setGeneratorId(p_result.generator_id());
    gen->setActive(p_result.active());
    gen->setTimestamp(to_rfc3339(p_result.timestamp().time_since_epoch()));
    gen->setStats(cpu_stats);
    return gen;
}

std::shared_ptr<CpuInfoResult> to_swagger(const model::cpu_info& p_cpu_info)
{
    auto cpu_info = std::make_shared<CpuInfoResult>();
    cpu_info->setArchitecture(p_cpu_info.architecture());
    cpu_info->setCacheLineSize(p_cpu_info.cache_line_size());
    cpu_info->setCores(p_cpu_info.cores());
    return cpu_info;
}

} // namespace openperf::cpu::api
