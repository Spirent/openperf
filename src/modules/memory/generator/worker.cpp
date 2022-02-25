#include <chrono>
#include <cstddef> //nullptr
#include <optional>
#include <variant>

#include "range/v3/all.hpp"

#include "core/op_core.h"
#include "memory/api.hpp"
#include "memory/generator/coordinator.hpp"
#include "memory/generator/worker_api.hpp"
#include "message/serialized_message.hpp"

namespace openperf::memory::generator::worker {

using namespace std::chrono_literals;

template <typename T> struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

struct state_started
{};

struct state_stopped
{};

using state = std::variant<state_stopped, state_started>;

template <typename Derived, typename StateVariant, typename EventVariant>
class finite_state_machine
{
    StateVariant m_state;

public:
    void dispatch(const EventVariant& event)
    {
        auto& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](auto& state, const auto& event) -> std::optional<StateVariant> {
                return (child.on_event(state, event));
            },
            m_state,
            event);

        if (next_state) { m_state = *next_state; };
    }

    int run()
    {
        auto& child = static_cast<Derived&>(*this);
        return (std::visit([&](auto& state) { return (child.on_run(state)); },
                           m_state));
    }
};

/* A 2 element vector in the mathematical sense that allows mixed types */
template <typename T, typename U>
constexpr auto vector(T&& a, U&& b)
    -> std::pair<remove_cvref_t<T>, remove_cvref_t<U>>
{
    return (std::make_pair(std::forward<T>(a), std::forward<U>(b)));
}

template <typename T, typename U> void dump_b(std::pair<T, U> p)
{
    std::cerr << p.first.count() << ", " << p.second << std::endl;
}

template <typename T, typename U> void dump(std::pair<T, U> p)
{
    std::cerr << p.first << ", " << p.second.count() << std::endl;
}

static size_t nb_blocks(const io_config& config)
{
    return (config.index_range.max - config.index_range.min);
}

/*
 * Workers are either readers or writers
 */

struct read_traits
{
    static constexpr auto thread_prefix = "op_mem_";
    static constexpr auto thread_suffix = "r";
    static constexpr auto io_clock_mask = 0xff;

    struct io_spinner
    {
        template <typename Timepoint>
        size_t operator()(const io_config& config,
                          const std::vector<uint32_t>& indexes,
                          std::byte* scratch,
                          size_t nb_ops,
                          size_t offset,
                          const Timepoint& limit)
        {
            using clock = result::clock;
            assert(offset < indexes.size());

            if (nb_blocks(config) == 0) { return (0); }

            auto nb_reads = 0UL;
            for (auto idx : ranges::views::cycle(indexes)
                                | ranges::views::drop(offset)
                                | ranges::views::take(nb_ops)) {
                std::copy_n(config.buffer + (idx * config.io_size),
                            config.io_size,
                            scratch);

                if ((++nb_reads & io_clock_mask) == 0 && clock::now() > limit) {
                    break;
                }
            }

            return (nb_reads);
        }
    };

    struct io_stats_extractor
    {
        template <typename Clock>
        io_stats<Clock>& operator()(struct thread_stats<Clock>& stats)
        {
            return (stats.read);
        }
    };

    struct io_load_extractor
    {
        ops_per_sec operator()(const struct update_msg& msg, uint8_t id)
        {
            assert(id < msg.load.read.size());
            return (msg.load.read[id]);
        }
    };
};

struct write_traits
{
    static constexpr auto thread_prefix = "op_mem_";
    static constexpr auto thread_suffix = "w";
    static constexpr auto io_clock_mask = 0xff;

    struct io_spinner
    {
        template <typename Timepoint>
        size_t operator()(const io_config& config,
                          const std::vector<uint32_t>& indexes,
                          const std::byte* scratch,
                          size_t nb_ops,
                          size_t offset,
                          const Timepoint& limit)
        {
            using clock = result::clock;
            assert(offset < indexes.size());

            if (nb_blocks(config) == 0) { return (0); }

            auto nb_writes = 0UL;

            for (auto idx : ranges::views::cycle(indexes)
                                | ranges::views::drop(offset)
                                | ranges::views::take(nb_ops)) {
                std::copy_n(scratch,
                            config.io_size,
                            config.buffer + (idx * config.io_size));

                if ((++nb_writes & io_clock_mask) == 0
                    && clock::now() > limit) {
                    break;
                }
            }

            return (nb_writes);
        }
    };

    struct io_stats_extractor
    {
        template <typename Clock>
        io_stats<Clock>& operator()(struct thread_stats<Clock>& stats)
        {
            return (stats.write);
        }
    };

    struct io_load_extractor
    {
        ops_per_sec operator()(const struct update_msg& msg, uint8_t id)
        {
            assert(id < msg.load.write.size());
            return (msg.load.write[id]);
        }
    };
};

template <typename Traits>
class worker : public finite_state_machine<worker<Traits>, state, command_msg>
{
    struct io_state
    {
        std::vector<uint32_t> indexes;
        uint32_t offset;
    };

    using clock = result::clock;
    struct io_stats
    {
        size_t operations;
        clock::duration run_time;
        clock::duration sleep_time;
        ops_per_sec avg_rate;
    };

    void* m_context;
    result* m_result;
    ops_per_sec m_rate;
    uint8_t m_id;
    io_config m_config;
    io_state m_state;
    io_stats m_stats;
    std::vector<std::byte> m_scratch;

    /* Handy definition to zero out stats */
    static constexpr auto io_stats_init = [](ops_per_sec rate) {
        return (io_stats{
            0, clock::duration::zero(), clock::duration::zero(), rate});
    };

    /*
     * We need these clock values to be double based.
     * Use a prime quanta value to avoid any sort of potential
     * synchronization with the OS scheduler.
     */
    static constexpr auto quanta =
        1. * std::chrono::duration_cast<clock::duration>(97ms);
    static constexpr auto zero = 1. * clock::duration::zero();

public:
    worker(void* context, const io_config& config, uint8_t id)
        : m_context(context)
        , m_result(nullptr)
        , m_rate(config.io_rate)
        , m_id(id)
        , m_config(config)
    {
        auto iota =
            ranges::views::iota(config.index_range.min, config.index_range.max);
        m_state.indexes.reserve(nb_blocks(config));

        switch (config.io_pattern) {
        case api::pattern_type::random:
            m_state.indexes |=
                ranges::actions::push_back(iota)
                | ranges::actions::shuffle(std::default_random_engine{});
        case api::pattern_type::reverse:
            m_state.indexes |=
                ranges::actions::push_back(iota | ranges::views::reverse);
        default:
            m_state.indexes |= ranges::actions::push_back(iota);
        }
        m_state.offset = 0;

        m_scratch.reserve(config.io_size);
        m_scratch |= ranges::actions::push_back(
            ranges::views::iota(uint8_t{0}, uint8_t{255}) | ranges::views::cycle
            | ranges::views::transform([](auto b) { return (std::byte{b}); })
            | ranges::views::take(config.io_size));
    }

    int on_run(const state_started&)
    {
        using io_spinner = typename Traits::io_spinner;
        using io_stats_extractor = typename Traits::io_stats_extractor;

        assert(m_result);
        assert(m_stats.avg_rate > ops_per_sec::zero());

        /*
         * Sleeping on some platforms is a dangerous proposition, as you
         * don't know if you're going to wake up any time close to what you
         * expect. Hence, we dynamically solve for the number of memory
         * operations to perform and the time to sleep using the following
         * system of equations:
         *
         * 1. (total ops + to do ops) /
         *      ((spin + sleep) + to do ops / avg_rate + sleep) = rate
         * 2. to_do_ops / total.avg_rate + sleep  = quanta
         *
         * We use Cramer's rule to solve for to_do_ops and sleep time.  We
         * are guaranteed a solution because our determinant is always 1.
         * However, our solution could be negative, so clamp our answers
         * before using.
         */

        auto a = vector(1. - (m_rate / m_stats.avg_rate),
                        units::to_duration<std::chrono::duration<double>>(
                            m_stats.avg_rate));
        auto b = vector(-m_rate, 1);
        auto c = vector(m_rate * (m_stats.run_time + m_stats.sleep_time)
                            - m_stats.operations,
                        quanta);

        auto to_do_ops = std::max(c.first * b.second - b.first * c.second, 0.);
        auto time_to_sleep =
            std::clamp(a.first * c.second - c.first * a.second, zero, quanta);

        assert(to_do_ops > 0);

        auto t1 = clock::now();
        clock::time_point t2;
        if (time_to_sleep > zero) {
            std::this_thread::sleep_until(t1 + time_to_sleep);
            t2 = clock::now();
            m_stats.sleep_time += t2 - t1;
        } else {
            t2 = t1;
        }

        auto done_ops = io_spinner{}(m_config,
                                     m_state.indexes,
                                     m_scratch.data(),
                                     to_do_ops,
                                     m_state.offset,
                                     t2 + quanta);

        auto t3 = clock::now();

        m_state.offset = (m_state.offset + done_ops) % m_state.indexes.size();

        auto& thread_stats = m_result->shard(m_id);
        if (!thread_stats.first()) { thread_stats.time_.first = t3; }
        thread_stats.time_.last = t3;

        /* Note: rate * duration -> scalar */
        auto run_time = t3 - t2;
        auto total_time = thread_stats.time_.last - thread_stats.time_.first;
        auto& io_stats = io_stats_extractor{}(thread_stats);
        io_stats.bytes_actual += done_ops * m_config.io_size;
        io_stats.bytes_target =
            m_config.io_rate * total_time * m_config.io_size;
        io_stats.latency += run_time;
        io_stats.ops_actual += done_ops;
        io_stats.ops_target = m_config.io_rate * total_time;

        // dump(std::cerr, thread_stats);

        m_stats.run_time += run_time;
        m_stats.operations += done_ops;

        return (0);
    }

    template <typename State> int on_run(const State&)
    {
        /* Nothing to do */
        return (-1);
    }

    /* State transition functions */
    template <typename State>
    std::optional<state> on_event(const State&, const start_msg& msg)
    {
        m_stats = io_stats_init(m_config.io_rate);
        m_result = msg.result;

        op_task_sync_ping(m_context, msg.endpoint.c_str());

        OP_LOG(OP_LOG_DEBUG, "Running\n");

        return (state_started{});
    }

    template <typename State>
    std::optional<state> on_event(const State&, const stop_msg& msg)
    {
        m_result = nullptr;

        op_task_sync_ping(m_context, msg.endpoint.c_str());

        OP_LOG(OP_LOG_DEBUG, "Stopped\n");

        return (state_stopped{});
    }

    template <typename State>
    std::optional<state> on_event(const State&, const update_msg& msg)
    {
        using load_extractor = typename Traits::io_load_extractor;
        auto new_rate = load_extractor{}(msg, m_id);

        OP_LOG(OP_LOG_DEBUG,
               "rate change %f -> %f\n",
               m_rate.count(),
               new_rate.count());

        m_rate = new_rate;
        m_stats = io_stats_init(m_config.io_rate);

        return (std::nullopt);
    }

    template <typename State, typename Message>
    std::optional<state> on_event(const State&, const Message&)
    {
        return (std::nullopt);
    }
};

template <typename Traits>
static int do_work(void* context,
                   uint8_t coordinator_id,
                   uint8_t thread_id,
                   const char* endpoint,
                   const io_config config,
                   std::promise<void> promise)
{
    op_thread_setname((Traits::thread_prefix + std::to_string(coordinator_id)
                       + "/" + Traits::thread_suffix
                       + std::to_string(thread_id))
                          .c_str());

    std::unique_ptr<void, op_socket_deleter> control(
        op_socket_get_client_subscription(context, endpoint, ""));

    /* Let the coordinator know that our socket is ready */
    promise.set_value();

    auto me = worker<Traits>(context, config, thread_id);

    while (auto cmd =
               openperf::message::recv(control.get()).and_then(deserialize)) {
        me.dispatch(*cmd);
        while (!op_socket_has_messages(control.get())) {
            if (auto error = me.run()) { break; }
        }
    }

    /* Cleanup */
    op_log_close();

    return (0);
}

int do_reads(void* context,
             uint8_t coordinator_id,
             uint8_t thread_id,
             const char* endpoint,
             io_config config,
             std::promise<void> promise)
{
    return (do_work<read_traits>(context,
                                 coordinator_id,
                                 thread_id,
                                 endpoint,
                                 config,
                                 std::move(promise)));
}

int do_writes(void* context,
              uint8_t coordinator_id,
              uint8_t thread_id,
              const char* endpoint,
              io_config config,
              std::promise<void> promise)
{
    return (do_work<write_traits>(context,
                                  coordinator_id,
                                  thread_id,
                                  endpoint,
                                  config,
                                  std::move(promise)));
}

} // namespace openperf::memory::generator::worker
