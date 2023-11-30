#include <cassert>
#include <future>

#include "core/op_core.h"
#include "cpu/api.hpp"
#include "cpu/generator/coordinator.hpp"
#include "cpu/generator/worker_api.hpp"

namespace openperf::cpu::generator {

static std::string get_unique_endpoint()
{
    static unsigned idx = 0;
    return ("inproc://op_cpu_endpoint_" + std::to_string(idx++));
}

template <typename Duration>
std::chrono::nanoseconds to_nanoseconds(const Duration& duration)
{
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
}

template <typename Duration>
std::chrono::duration<double> to_seconds(const Duration& duration)
{
    return (
        std::chrono::duration_cast<std::chrono::duration<double>>(duration));
}

template <typename Duration>
static constexpr uint64_t to_timeout(const Duration& d)
{
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(d).count());
}

/*
 * Convert data type, instruction set pairs into a target array index
 */

template <typename InstructionSet>
std::optional<target_stats> to_target_stats(api::data_type data)
{
    switch (data) {
    case api::data_type::int32:
        return (target_stats_impl<InstructionSet, int32_t>{});
    case api::data_type::int64:
        return (target_stats_impl<InstructionSet, int64_t>{});
    case api::data_type::float32:
        return (target_stats_impl<InstructionSet, float>{});
    case api::data_type::float64:
        return (target_stats_impl<InstructionSet, double>{});
    default:
        return (std::nullopt);
    }
}

static std::optional<target_stats> to_target_stats(api::instruction_type instr,
                                                   api::data_type data)
{
    switch (instr) {
    case api::instruction_type::scalar:
        return (to_target_stats<instruction_set::scalar>(data));
    case api::instruction_type::sse2:
        return (to_target_stats<instruction_set::sse2>(data));
    case api::instruction_type::sse4:
        return (to_target_stats<instruction_set::sse4>(data));
    case api::instruction_type::avx:
        return (to_target_stats<instruction_set::avx>(data));
    case api::instruction_type::avx2:
        return (to_target_stats<instruction_set::avx2>(data));
    case api::instruction_type::avx512:
        return (to_target_stats<instruction_set::avx512>(data));
    case api::instruction_type::neon:
        return (to_target_stats<instruction_set::neon>(data));
    default:
        return (std::nullopt);
    }
}

static std::optional<size_t> get_target_index(std::string_view name)
{
    static constexpr auto delimiters = ", ";
    auto tokens = std::vector<std::string_view>{};
    size_t beg = 0, pos = 0;
    while ((beg = name.find_first_not_of(delimiters, pos))
           != std::string::npos) {
        pos = name.find_first_of(delimiters, beg + 1);
        tokens.emplace_back(name.substr(beg, pos - beg));
    }

    if (tokens.size() != 2) { return (std::nullopt); }

    auto stats = to_target_stats(api::to_instruction_type(tokens[0]),
                                 api::to_data_type(tokens[1]));
    if (!stats) { return (std::nullopt); }

    return (stats->index());
}

static std::optional<double> shard_extractor(const result::core_shard& stat,
                                             std::string_view name)
{
    if (name == "timestamp") {
        /*
         * XXX: We need to bypass the sanity check for last(),
         * on the first read hence, we get the "timestamp" value directly.
         */
        return (to_nanoseconds(stat.time_.last.time_since_epoch()).count());
    }
    if (name == "available") { return (to_seconds(stat.available()).count()); }
    if (name == "utilization") {
        return (to_seconds(stat.utilization()).count());
    }
    if (name == "target") { return (to_seconds(stat.target).count()); }
    if (name == "system") { return (to_seconds(stat.system).count()); }
    if (name == "user") { return (to_seconds(stat.user).count()); }
    if (name == "steal") { return (to_seconds(stat.steal()).count()); }
    if (name == "error") { return (to_seconds(stat.error()).count()); }

    /* Parse the targets[N] field name */
    constexpr std::string_view prefix = "targets[";
    constexpr auto prefix_size = prefix.length();

    if (name.substr(0, prefix_size) == prefix) {
        auto target_number = get_target_index(name.substr(
            prefix_size, name.find_first_of(']', prefix_size) - prefix_size));
        if (!target_number) { return (std::nullopt); }
        if (target_number.value() < stat.targets.size()) {
            auto target_field =
                name.substr(name.find_first_of('.', prefix_size) + 1);
            if (target_field == "operations") {
                return (std::visit([](const auto& t) { return (t.operations); },
                                   stat.targets[target_number.value()]));
            }
        }
    }

    return (std::nullopt);
}

static std::optional<double>
result_extractor(const std::vector<result::core_shard>& shards,
                 std::string_view name)
{
    if (name == "timestamp" || name == "available" || name == "utilization"
        || name == "target" || name == "system" || name == "user"
        || name == "steal" || name == "error") {
        return (shard_extractor(sum_stats(shards), name));
    }

    /* Parse the cores[N] field name */
    constexpr std::string_view prefix = "cores[";
    constexpr auto prefix_size = prefix.length();

    if (name.substr(0, prefix_size) == prefix) {
        auto core_number = std::stoul(std::string(name.substr(
            prefix_size, name.find_first_of(']', prefix_size) - prefix_size)));
        if (core_number < shards.size()) {
            auto field_name =
                name.substr(name.find_first_of('.', prefix_size) + 1);
            return (shard_extractor(shards[core_number], field_name));
        }
    }

    /*
     * XXX: safer to return 0 here than nullopt. The dynamic spooler
     * attempts to verify stats existence at runtime and will throw
     * exceptions if we return a nullopt here for a non-existent stat.
     */
    return (0);
}

/* XXX: Initialize core shard with values so that results are 0'd out */
result::result(size_t size, const dynamic::configuration& dynamic_config)
    : m_shards(size,
               core_shard{result::clock::now(), generator::get_steal_time()})
    , m_dynamic(result_extractor)
{
    m_dynamic.configure(dynamic_config, shards());
}

result::core_shard& result::shard(size_t idx)
{
    assert(idx < m_shards.size());
    return (m_shards[idx]);
}

const std::vector<result::core_shard>& result::shards() const
{
    return (m_shards);
}

dynamic::results result::dynamic() const { return (m_dynamic.result()); }

void result::do_dynamic_update() { m_dynamic.add(shards()); }

static uint8_t coordinator_id()
{
    static uint8_t idx = 0;
    return (idx++);
}

coordinator::coordinator(void* context, struct config&& conf)
    : m_context(context)
    , m_client(context, get_unique_endpoint().c_str())
    , m_config(conf)
    , m_loop_id(std::nullopt)
    , m_prev_sum(std::nullopt)
    , m_pid(0.07, 1.0 / 9, 0)
{
    assert(m_config.cores.has_value() == false
           || m_config.core_configs.size() <= m_config.cores->count());

    m_setpoint = std::accumulate(std::begin(m_config.core_configs),
                                 std::end(m_config.core_configs),
                                 0.,
                                 [](double sum, const auto& config) {
                                     return (sum + config.utilization);
                                 });

    m_pid.max(100. * m_config.core_configs.size());

    /* Spawn a thread for each core configuration */
    const auto endpoint = m_client.endpoint();
    const auto id = coordinator_id();
    auto start_results = std::vector<std::future<void>>{};
    auto core_id = 0;

    for (auto&& config : m_config.core_configs) {
        auto start_token = std::promise<void>{};
        start_results.push_back(start_token.get_future());

        m_threads.emplace_back(std::thread(worker::do_work,
                                           m_context,
                                           id,
                                           core_id,
                                           config,
                                           endpoint.c_str(),
                                           std::move(start_token)));
        core_id++;
    }

    /*
     * Wait for threads to start. If we don't, we could attempt to
     * send them a socket message before they've constructed their
     * listening socket. That would be bad. :(
     */
    std::for_each(std::begin(start_results),
                  std::end(start_results),
                  [](auto&& token) { token.wait(); });
}

coordinator::~coordinator()
{
    m_client.terminate();

    std::for_each(std::begin(m_threads),
                  std::end(m_threads),
                  [](auto&& thread) { thread.join(); });
}

const struct config& coordinator::config() const { return (m_config); }

bool coordinator::active() const { return (result() != nullptr); }

result* coordinator::result() const { return (m_results.get()); }

static int handle_timeout(const op_event_data*, void* arg)
{
    auto* c = static_cast<coordinator*>(arg);
    return (c->do_load_update());
}

std::shared_ptr<result>
coordinator::start(core::event_loop& loop,
                   const dynamic::configuration& dynamic_config)
{
    assert(!m_loop_id.has_value());
    assert(!m_prev_sum.has_value());

    using namespace std::chrono_literals;

    m_results = std::make_shared<generator::result>(
        m_config.core_configs.size(), dynamic_config);

    m_pid.reset(m_setpoint);
    m_pid.start();
    m_prev_sum = result::core_shard{};

    m_client.start(m_context, m_results.get(), m_config.core_configs.size());

    auto callbacks = op_event_callbacks{.on_timeout = handle_timeout};

    uint32_t id;
    loop.add(to_timeout(1s), &callbacks, this, &id);
    m_loop_id = id;

    return (m_results);
}

void coordinator::stop(core::event_loop& loop)
{
    if (m_loop_id.has_value()) {

        m_client.stop(m_context, m_config.core_configs.size());

        m_pid.update(0);

        loop.del(m_loop_id.value());
        m_loop_id = std::nullopt;
        m_prev_sum = std::nullopt;

        m_results.reset();
    }
}

template <typename T> struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

int coordinator::do_load_update()
{
    if (!m_results) {
        // Prevent crash by validating m_results is still valid.

        // Originally event_loop del() did not prevent callbacks from pending
        // events. This should not happen anymore.
        OP_LOG(OP_LOG_ERROR, "do_load_update called with no active results");
        return 0;
    }

    assert(m_prev_sum);

    m_results->do_dynamic_update();
    const auto& shards = m_results->shards();
    assert(shards.size());

    /* Collect load from workers */
    auto sum = sum_stats(shards);
    auto load = (sum.utilization() - m_prev_sum->utilization()) * 100.0
                / (sum.available() - m_prev_sum->available());

    /* Update PID loop */
    auto adjust = m_pid.stop(load);
    m_prev_sum = sum;

    /* Adjust worker load */
    auto total = std::accumulate(std::begin(m_config.core_configs),
                                 std::end(m_config.core_configs),
                                 0.,
                                 [](double sum, const auto& config) {
                                     return (sum + config.utilization);
                                 });

    auto loads = std::vector<double>{};
    std::transform(std::begin(m_config.core_configs),
                   std::end(m_config.core_configs),
                   std::back_inserter(loads),
                   [&](const auto& config) {
                       return (config.utilization * (1 + adjust / total));
                   });

    /* Update workers and restart PID controller */
    m_client.update(std::move(loads));

    m_pid.start();

    return (0);
}

} // namespace openperf::cpu::generator
