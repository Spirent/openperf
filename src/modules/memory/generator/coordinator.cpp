#include <cassert>

#include "core/op_core.h"
#include "memory/api.hpp"
#include "memory/generator/coordinator.hpp"
#include "memory/generator/worker_api.hpp"
#include "utils/associative_array.hpp"

namespace openperf::memory::generator {

static std::string get_unique_endpoint()
{
    static unsigned idx = 0;
    return ("inproc://op_mem_endpoint_" + std::to_string(idx++));
}

template <typename Duration>
std::chrono::nanoseconds to_nanoseconds(const Duration& duration)
{
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
}

static std::optional<double>
result_extractor(const std::vector<result::thread_shard>& shards,
                 std::string_view name)
{
    using io_stats = io_stats<result::clock>;

    static auto io_stat_extractors =
        utils::associative_array<std::string_view,
                                 std::function<double(const io_stats&)>>(
            std::pair("bytes_actual",
                      [](const io_stats& s) { return (s.ops_actual); }),
            std::pair("bytes_target",
                      [](const io_stats& s) { return (s.ops_target); }),
            std::pair("latency_total",
                      [](const io_stats& s) {
                          return (to_nanoseconds(s.latency).count());
                      }),
            std::pair("ops_actual",
                      [](const io_stats& s) { return (s.ops_actual); }),
            std::pair("ops_target",
                      [](const io_stats& s) { return (s.ops_target); }));

    auto stats = sum_stats(shards);

    if (name == "timestamp") {
        return (to_nanoseconds(stats.time_.last.time_since_epoch()).count());
    }

    /* Parse name to determine read/write */
    constexpr std::string_view read_prefix = "read.";
    constexpr std::string_view write_prefix = "write.";

    if (name.substr(0, read_prefix.length()) == read_prefix) {
        auto stat_name = name.substr(read_prefix.length());
        if (auto f = utils::key_to_value(io_stat_extractors, stat_name)) {
            return ((*f)(stats.read));
        }
        return (std::nullopt);
    } else if (name.substr(0, write_prefix.length()) == write_prefix) {
        auto stat_name = name.substr(write_prefix.length());
        if (auto f = utils::key_to_value(io_stat_extractors, stat_name)) {
            return ((*f)(stats.write));
        }
        return (std::nullopt);
    }

    return (0);
}

result::result(size_t size, const dynamic::configuration& dynamic_config)
    : m_shards(size, thread_shard{result::clock::now()})
    , m_dynamic(result_extractor)
{
    m_dynamic.configure(dynamic_config, shards());
}

result::thread_shard& result::shard(size_t idx)
{
    assert(idx < m_shards.size());
    return (m_shards[idx]);
}

const std::vector<result::thread_shard>& result::shards() const
{
    return (m_shards);
}

dynamic::results result::dynamic() const { return (m_dynamic.result()); }

void result::do_dynamic_update() { m_dynamic.add(shards()); }

/**
 * Get the count of items in bucket 'n' if we evenly distribute a 'total' count
 * across the specified number of 'buckets'.
 * Obviously, 0 <= n < buckets.
 */
template <typename T, typename U, typename V>
constexpr T distribute(T total, U buckets, V n)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_integral_v<U>);
    static_assert(std::is_integral_v<V>);

    assert(n < buckets);
    auto base = total / buckets;
    return (n < total % buckets ? base + 1 : base);
}

template <typename U, typename V>
constexpr ops_per_sec rate_distribute(ops_per_sec rate, U buckets, V n)
{
    static_assert(std::is_integral_v<U>);
    static_assert(std::is_integral_v<V>);

    assert(n < buckets);
    auto total = static_cast<int64_t>(std::trunc(rate.count()));
    return (ops_per_sec{distribute(total, buckets, n)});
}

static uint8_t coordinator_id()
{
    static uint8_t idx = 0;
    return (idx++);
}

static size_t get_thread_count(const io_config& config)
{
    return (config.io_rate > ops_per_sec::zero() ? config.io_threads : 0);
}

static size_t get_thread_count(const config& c)
{
    return (get_thread_count(c.read) + get_thread_count(c.write));
}

coordinator::coordinator(void* context, struct config&& conf)
    : m_context(context)
    , m_client(context, get_unique_endpoint().c_str())
    , m_config(conf)
    , m_loop_id(std::nullopt)
    , m_prev_sum(std::nullopt)
    , m_pids{.read = pid_controller(0.07, 1. / 9, 0),
             .write = pid_controller(0.07, 1. / 9, 0)}
    , m_buffer(conf.buffer_size)
{
    const auto endpoint = m_client.endpoint();
    const auto id = coordinator_id();
    auto start_results = std::vector<std::future<void>>{};

    /* Start readers */
    std::generate_n(
        std::back_inserter(m_threads),
        get_thread_count(m_config.read),
        [&, idx = 0U, offset = 0U]() mutable {
            auto buffer_blocks = m_config.buffer_size / m_config.read.io_size;
            auto nb_blocks =
                distribute(buffer_blocks, m_config.read.io_threads, idx);

            auto config = worker::io_config{
                .buffer = m_buffer.data(),
                .index_range = {.min = offset, .max = offset + nb_blocks},
                .io_size = m_config.read.io_size,
                .io_rate = rate_distribute(
                    m_config.read.io_rate, m_config.read.io_threads, idx),
                .io_pattern = m_config.read.io_pattern};

            offset += nb_blocks;
            auto start_token = std::promise<void>{};
            start_results.push_back(start_token.get_future());

            return (std::thread(worker::do_reads,
                                m_context,
                                id,
                                idx++,
                                endpoint.c_str(),
                                config,
                                std::move(start_token)));
        });

    /* Start writers */
    std::generate_n(
        std::back_inserter(m_threads),
        get_thread_count(m_config.write),
        [&, idx = 0U, offset = 0U]() mutable {
            auto buffer_blocks = m_config.buffer_size / m_config.write.io_size;
            auto nb_blocks =
                distribute(buffer_blocks, m_config.write.io_threads, idx);

            auto config = worker::io_config{
                .buffer = m_buffer.data(),
                .index_range = {.min = offset, .max = offset + nb_blocks},
                .io_size = m_config.write.io_size,
                .io_rate = rate_distribute(
                    m_config.write.io_rate, m_config.write.io_threads, idx),
                .io_pattern = m_config.write.io_pattern};

            offset += nb_blocks;
            auto start_token = std::promise<void>{};
            start_results.push_back(start_token.get_future());

            return (std::thread(worker::do_writes,
                                m_context,
                                id,
                                idx++,
                                endpoint.c_str(),
                                config,
                                std::move(start_token)));
        });

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

template <typename Duration>
static constexpr uint64_t to_timeout(const Duration& d)
{
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(d).count());
}

std::shared_ptr<result>
coordinator::start(core::event_loop& loop,
                   const dynamic::configuration& dynamic_config)
{
    assert(!m_loop_id.has_value());
    assert(!m_prev_sum.has_value());

    using namespace std::chrono_literals;

    m_results = std::make_shared<generator::result>(get_thread_count(m_config),
                                                    dynamic_config);

    m_pids.read.max(m_config.read.io_rate.count() * 2);
    m_pids.read.reset(m_config.read.io_rate.count());
    m_pids.read.start();

    m_pids.write.max(m_config.write.io_rate.count() * 2);
    m_pids.write.reset(m_config.write.io_rate.count());
    m_pids.write.start();

    m_prev_sum = result::thread_shard{};

    m_client.start(m_context, m_results.get(), get_thread_count(m_config));

    auto callbacks =
        op_event_callbacks{.on_timeout = [](const op_event_data*, void* arg) {
            auto* c = static_cast<coordinator*>(arg);
            return (c->do_load_update());
        }};
    uint32_t id = 0;

    loop.add(to_timeout(1s), &callbacks, this, &id);
    m_loop_id = id;

    return (m_results);
}

void coordinator::stop(core::event_loop& loop)
{
    if (m_loop_id.has_value()) {
        m_client.stop(m_context, get_thread_count(m_config));

        m_pids.read.update(0);
        m_pids.write.update(0);

        loop.del(m_loop_id.value());
        m_loop_id = std::nullopt;
        m_prev_sum = std::nullopt;

        dump(std::cerr, sum_stats(m_results->shards()));
        m_results.reset();
    }
}

template <typename Duration>
std::chrono::duration<double> to_seconds(const Duration& duration)
{
    return (
        std::chrono::duration_cast<std::chrono::duration<double>>(duration));
}

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

    /* Calculate worker load */
    auto sum = sum_stats(shards);
    if (auto last = sum.last()) {
        auto delta_t =
            last.value() - m_prev_sum->last().value_or(sum.first().value());
        auto read_rate = (sum.read.ops_actual - m_prev_sum->read.ops_actual)
                         / to_seconds(delta_t).count();
        auto write_rate = (sum.write.ops_actual = m_prev_sum->write.ops_actual)
                          / to_seconds(delta_t).count();

        /* Update PID controllers */
        auto read_adjust = ops_per_sec{m_pids.read.stop(read_rate)};
        auto write_adjust = ops_per_sec{m_pids.write.stop(write_rate)};

        /* Generate updated load vectors */
        auto read_load = std::vector<ops_per_sec>{};
        std::generate_n(
            std::back_inserter(read_load),
            get_thread_count(m_config.read),
            [&, idx = 0U]() mutable {
                return (rate_distribute(m_config.read.io_rate + read_adjust,
                                        m_config.read.io_threads,
                                        idx++));
            });

        auto write_load = std::vector<ops_per_sec>{};
        std::generate_n(
            std::back_inserter(write_load),
            get_thread_count(m_config.write),
            [&, idx = 0U]() mutable {
                return (rate_distribute(m_config.write.io_rate + write_adjust,
                                        m_config.write.io_threads,
                                        idx++));
            });

        /* Update workers and restart PID controllers */
        m_client.update(worker::io_load{.read = std::move(read_load),
                                        .write = std::move(write_load)});
    }

    m_pids.read.start();
    m_pids.write.start();

    return (0);
}

} // namespace openperf::memory::generator
