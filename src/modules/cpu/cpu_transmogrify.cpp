#include <chrono>
#include <future>
#include <iomanip>

#include "cpu/api.hpp"
#include "cpu/arg_parser.hpp"
#include "cpu/generator/config.hpp"
#include "cpu/generator/coordinator.hpp"

#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"

namespace openperf::cpu::api {

static generator::config
from_swagger(const swagger::v1::model::CpuGeneratorSystemConfig& src)
{
    using system_target_config_type =
        generator::target_operations_config_impl<instruction_set::scalar,
                                                 int64_t>;

    auto cores = config::core_mask();
    auto core_count = cores ? cores->count() : core::cpuset_online().count();
    auto per_core_load = src.getUtilization() / core_count;

    assert(per_core_load <= 100.0);

    auto dst = generator::config{.cores = cores, .is_system_config = true};
    std::generate_n(std::back_inserter(dst.core_configs), core_count, [&]() {
        auto cc = generator::core_config{};
        cc.utilization = per_core_load;
        cc.targets.emplace_back(system_target_config_type{1});
        return (cc);
    });

    return (dst);
}

template <typename InstructionSet>
generator::target_op_config to_target_op_config(enum data_type data, int weight)
{
    switch (data) {
    case data_type::int32:
        return (
            generator::target_operations_config_impl<InstructionSet, int32_t>{
                weight});
    case data_type::float32:
        return (generator::target_operations_config_impl<InstructionSet, float>{
            weight});
    case data_type::float64:
        return (
            generator::target_operations_config_impl<InstructionSet, double>{
                weight});
    default:
        return (
            generator::target_operations_config_impl<InstructionSet, int64_t>{
                weight});
    }
}

static generator::target_op_config
from_swagger(const swagger::v1::model::CpuGeneratorCoreConfig_targets& src)
{
    auto instr = to_instruction_type(src.getInstructionSet());
    auto data = to_data_type(src.getDataType());

    switch (instr) {
    case instruction_type::sse2:
        return (
            to_target_op_config<instruction_set::sse2>(data, src.getWeight()));
    case instruction_type::sse4:
        return (
            to_target_op_config<instruction_set::sse4>(data, src.getWeight()));
    case instruction_type::avx:
        return (
            to_target_op_config<instruction_set::avx>(data, src.getWeight()));
    case instruction_type::avx2:
        return (
            to_target_op_config<instruction_set::avx2>(data, src.getWeight()));
    case instruction_type::avx512:
        return (to_target_op_config<instruction_set::avx512>(data,
                                                             src.getWeight()));
    case instruction_type::neon:
        return (
            to_target_op_config<instruction_set::neon>(data, src.getWeight()));
    default:
        return (to_target_op_config<instruction_set::scalar>(data,
                                                             src.getWeight()));
    }
}

static generator::core_config
from_swagger(const swagger::v1::model::CpuGeneratorCoreConfig& src)
{
    auto dst = generator::core_config{};
    dst.utilization = src.getUtilization();

    const auto& targets =
        const_cast<swagger::v1::model::CpuGeneratorCoreConfig&>(src)
            .getTargets();
    std::transform(std::begin(targets),
                   std::end(targets),
                   std::back_inserter(dst.targets),
                   [](const auto& target) { return (from_swagger(*target)); });

    return (dst);
}

generator::config
from_swagger(const swagger::v1::model::CpuGeneratorConfig& src)
{
    auto method = to_method_type(src.getMethod());

    switch (method) {
    case method_type::system: {
        auto system = src.getSystem();
        return (from_swagger(*system));
    }
    case method_type::cores: {
        auto dst = generator::config{.cores = config::core_mask(),
                                     .is_system_config = false};
        const auto& core_configs =
            const_cast<swagger::v1::model::CpuGeneratorConfig&>(src).getCores();
        assert(core_configs.size() <= op_cpu_count());
        std::transform(
            std::begin(core_configs),
            std::end(core_configs),
            std::back_inserter(dst.core_configs),
            [](const auto& config) { return (from_swagger(*config)); });
        return (dst);
    }
    default:
        throw std::runtime_error("Unrecognized generator config");
    }
}

template <typename SwaggerObject,
          template <typename, typename>
          class Target,
          typename Variant>
std::shared_ptr<SwaggerObject> to_swagger_load_object(const Variant& src)
{
    auto dst = std::make_shared<SwaggerObject>();

    auto [instr, data] = std::visit(
        utils::overloaded_visitor(
            [](const Target<instruction_set::scalar, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::scalar),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::scalar, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::scalar),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::scalar, float>&) {
                return (std::make_pair(to_string(instruction_type::scalar),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::scalar, double>&) {
                return (std::make_pair(to_string(instruction_type::scalar),
                                       to_string(data_type::float64)));
            },
            [](const Target<instruction_set::sse2, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::sse2),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::sse2, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::sse2),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::sse2, float>&) {
                return (std::make_pair(to_string(instruction_type::sse2),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::sse2, double>&) {
                return (std::make_pair(to_string(instruction_type::sse2),
                                       to_string(data_type::float64)));
            },
            [](const Target<instruction_set::sse4, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::sse4),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::sse4, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::sse4),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::sse4, float>&) {
                return (std::make_pair(to_string(instruction_type::sse4),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::sse4, double>&) {
                return (std::make_pair(to_string(instruction_type::sse4),
                                       to_string(data_type::float64)));
            },
            [](const Target<instruction_set::avx, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::avx),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::avx, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::avx),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::avx, float>&) {
                return (std::make_pair(to_string(instruction_type::avx),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::avx, double>&) {
                return (std::make_pair(to_string(instruction_type::avx),
                                       to_string(data_type::float64)));
            },
            [](const Target<instruction_set::avx2, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::avx2),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::avx2, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::avx2),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::avx2, float>&) {
                return (std::make_pair(to_string(instruction_type::avx2),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::avx2, double>&) {
                return (std::make_pair(to_string(instruction_type::avx2),
                                       to_string(data_type::float64)));
            },
            [](const Target<instruction_set::avx512, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::avx512),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::avx512, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::avx512),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::avx512, float>&) {
                return (std::make_pair(to_string(instruction_type::avx512),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::avx512, double>&) {
                return (std::make_pair(to_string(instruction_type::avx512),
                                       to_string(data_type::float64)));
            },
            [](const Target<instruction_set::neon, int32_t>&) {
                return (std::make_pair(to_string(instruction_type::neon),
                                       to_string(data_type::int32)));
            },
            [](const Target<instruction_set::neon, int64_t>&) {
                return (std::make_pair(to_string(instruction_type::neon),
                                       to_string(data_type::int64)));
            },
            [](const Target<instruction_set::neon, float>&) {
                return (std::make_pair(to_string(instruction_type::neon),
                                       to_string(data_type::float32)));
            },
            [](const Target<instruction_set::neon, double>&) {
                return (std::make_pair(to_string(instruction_type::neon),
                                       to_string(data_type::float64)));
            }),
        src);

    dst->setInstructionSet(instr);
    dst->setDataType(data);

    return (dst);
}

static std::shared_ptr<swagger::v1::model::CpuGeneratorCoreConfig>
to_swagger(const generator::core_config& config)
{
    auto dst = std::make_shared<swagger::v1::model::CpuGeneratorCoreConfig>();

    dst->setUtilization(config.utilization);

    std::transform(
        std::begin(config.targets),
        std::end(config.targets),
        std::back_inserter(dst->getTargets()),
        [](const auto& target) {
            auto t = to_swagger_load_object<
                swagger::v1::model::CpuGeneratorCoreConfig_targets,
                generator::target_operations_config_impl>(target);
            t->setWeight(std::visit(
                [](const auto& impl) { return (impl.weight); }, target));
            return (t);
        });

    return (dst);
}

static std::shared_ptr<swagger::v1::model::CpuGeneratorConfig>
to_swagger(const generator::config& src)
{
    auto dst = std::make_shared<swagger::v1::model::CpuGeneratorConfig>();
    if (src.is_system_config) {
        dst->setMethod(to_string(method_type::system));

        auto system =
            std::make_shared<swagger::v1::model::CpuGeneratorSystemConfig>();
        system->setUtilization(
            std::accumulate(std::begin(src.core_configs),
                            std::end(src.core_configs),
                            0.,
                            [](double sum, const auto& config) {
                                return (sum + config.utilization);
                            }));
        dst->setSystem(system);
    } else {
        dst->setMethod(to_string(method_type::cores));
        std::transform(std::begin(src.core_configs),
                       std::end(src.core_configs),
                       std::back_inserter(dst->getCores()),
                       [](const auto& config) { return (to_swagger(config)); });
    }

    return (dst);
}

cpu_generator_ptr to_swagger(std::string_view id,
                             const generator::coordinator& gen)
{
    auto dst = std::make_unique<swagger::v1::model::CpuGenerator>();
    dst->setId(std::string(id));
    dst->setConfig(to_swagger(gen.config()));
    dst->setRunning(gen.active());

    return (dst);
}

template <typename Clock>
std::string to_rfc3339(std::chrono::time_point<Clock> from)
{
    using namespace std::chrono_literals;
    static constexpr auto ns_per_sec = std::chrono::nanoseconds(1s).count();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        from.time_since_epoch())
                        .count();

    time_t sec = total_ns / ns_per_sec;
    auto ns = total_ns % ns_per_sec;

    std::stringstream os;
    os << std::put_time(gmtime(&sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(9) << ns << "Z";
    return (os.str());
}

template <typename Duration>
std::chrono::nanoseconds to_nanoseconds(const Duration& duration)
{
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
}

static std::shared_ptr<swagger::v1::model::CpuGeneratorTargetStats>
to_swagger(const target_stats& src)
{
    auto dst =
        to_swagger_load_object<swagger::v1::model::CpuGeneratorTargetStats,
                               target_stats_impl>(src);
    dst->setOperations(
        std::visit([](const auto& stat) { return (stat.operations); }, src));

    return (dst);
}

static std::shared_ptr<swagger::v1::model::CpuGeneratorCoreStats>
to_swagger(const generator::result::core_shard& src)
{
    auto dst = std::make_shared<swagger::v1::model::CpuGeneratorCoreStats>();

    dst->setAvailable(to_nanoseconds(src.available()).count());
    dst->setError(to_nanoseconds(src.error()).count());
    dst->setSteal(to_nanoseconds(src.steal).count());
    dst->setSystem(to_nanoseconds(src.system).count());
    dst->setTarget(to_nanoseconds(src.target).count());
    dst->setUser(to_nanoseconds(src.user).count());
    dst->setUtilization(to_nanoseconds(src.utilization()).count());

    auto& targets = dst->getTargets();
    std::transform(std::begin(src.targets),
                   std::end(src.targets),
                   std::back_inserter(targets),
                   [](const auto& t) { return (to_swagger(t)); });

    return (dst);
}

static std::shared_ptr<swagger::v1::model::CpuGeneratorStats>
to_swagger(const std::vector<generator::result::core_shard>& shards,
           const generator::result::core_shard& sum)
{
    auto dst = std::make_shared<swagger::v1::model::CpuGeneratorStats>();

    dst->setAvailable(to_nanoseconds(sum.available() * shards.size()).count());
    dst->setError(to_nanoseconds(sum.error()).count());
    dst->setSteal(to_nanoseconds(sum.steal).count());
    dst->setSystem(to_nanoseconds(sum.system).count());
    dst->setTarget(to_nanoseconds(sum.target).count());
    dst->setUser(to_nanoseconds(sum.user).count());
    dst->setUtilization(to_nanoseconds(sum.utilization()).count());

    auto& cores = dst->getCores();
    std::transform(std::begin(shards),
                   std::end(shards),
                   std::back_inserter(cores),
                   [](const auto& c) { return (to_swagger(c)); });

    return (dst);
}

cpu_generator_result_ptr
to_swagger(std::string_view generator_id,
           std::string_view result_id,
           const std::shared_ptr<generator::result>& result)
{
    auto dst = std::make_unique<swagger::v1::model::CpuGeneratorResult>();
    dst->setId(std::string(result_id));
    dst->setGeneratorId(std::string(generator_id));

    /* Result is active iff coordinator and server both have pointers to it */
    dst->setActive(result.use_count() > 1);

    const auto& shards = result->shards();
    auto sum = std::accumulate(std::begin(shards),
                               std::end(shards),
                               generator::result::core_shard{},
                               std::plus<>{});

    dst->setTimestampFirst(to_rfc3339(sum.first_));
    dst->setTimestampLast(to_rfc3339(sum.last_));
    dst->setStats(to_swagger(shards, sum));
    dst->setDynamicResults(std::make_shared<swagger::v1::model::DynamicResults>(
        to_swagger(result->dynamic())));

    return (dst);
}

} // namespace openperf::cpu::api
