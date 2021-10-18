#include "core/op_core.h"
#include "cpu/generator/config.hpp"
#include "cpu/generator/coordinator.hpp"
#include "cpu/generator/matrix_functions.hpp"
#include "cpu/generator/system_stats.hpp"
#include "cpu/generator/target.hpp"
#include "cpu/generator/worker_api.hpp"
#include "message/serialized_message.hpp"
#include "utils/variant_index.hpp"

namespace openperf::cpu::generator::worker {

using namespace std::chrono_literals;

template <typename T> struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

unsigned spin(const std::vector<target_op>& targets, target_stats results[])
{
    static_assert(
        std::variant_size_v<target_op> == std::variant_size_v<target_stats>);

    unsigned ops = 0;
    std::for_each(
        std::begin(targets), std::end(targets), [&](const auto& target) {
            ops += std::visit(
                [&](const auto& op) {
                    auto nb_ops = op();
                    auto index =
                        utils::variant_index<target_op,
                                             remove_cvref_t<decltype(op)>>();
                    std::visit([&](auto& stat) { stat.operations += nb_ops; },
                               results[index]);
                    return (nb_ops);
                },
                target);
        });

    return (ops);
}

template <typename Duration>
static void spin_for(const Duration& duration,
                     const std::vector<target_op>& targets,
                     target_stats results[])
{
    auto deadline = result::clock::now() + duration;

    do {
        spin(targets, results);
        spin(targets, results);
        spin(targets, results);
        spin(targets, results);
    } while (result::clock::now() < deadline);
}

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

/* A 2 element vector in the mathematical sense */
template <typename T, typename U>
constexpr auto vector(T&& a, U&& b) -> std::array<remove_cvref_t<T>, 2>
{
    return {{std::forward<T>(a), std::forward<U>(b)}};
}

class worker : public finite_state_machine<worker, state, command_msg>
{
    void* m_context;
    result* m_result;
    double m_ratio;
    std::vector<target_op> m_ops;
    uint8_t m_id;

    using clock = result::clock;
    struct stats
    {
        clock::duration run;
        clock::duration sleep;
    };
    stats m_time;

    utilization_time<clock> m_ref;

    /* We need these clock based values to be double based */
    static constexpr auto quanta =
        1. * std::chrono::duration_cast<clock::duration>(10ms);
    static constexpr auto zero = 1. * clock::duration::zero();

public:
    worker(void* context, const core_config& config, uint8_t id)
        : m_context(context)
        , m_result(nullptr)
        , m_ratio(config.utilization / 100.)
        , m_id(id)
    {
        assert(m_ratio <= 1.);

        /*
         * Translate target configuration into an array
         * of operations objects.
         */
        std::for_each(
            std::begin(config.targets),
            std::end(config.targets),
            [&](const auto& target_config) {
                std::visit(
                    [&](const auto& config) {
                        using config_type = remove_cvref_t<decltype(config)>;
                        using op_type = target_op_impl<
                            typename config_type::instruction_set_type,
                            typename config_type::data_type>;

                        for (auto i = 0; i < config.weight; i++) {
                            m_ops.emplace_back(op_type{});
                        }
                    },
                    target_config);
            });
    }

    /* State event functions */
    int on_run(const state_started&)
    {
        assert(m_result);

        /*
         * Sleeping on some platforms is a dangerous proposition, as you don't
         * know if you're going to wake up any time close to what you expect.
         * Hence, we dynamically solve for our spin/sleep times using the
         * following system of equations:
         *
         * 1. (total spin + spin) / (total sleep + sleep) = ratio / (1 - ratio)
         * 2. spin + sleep = QUANTA
         *
         * We use Cramer's rule to solve for the variables spin and sleep.  Note
         * that we are always guaranteed a solution, as the determinant of our
         * 2 x 2 matrix is always 1.  Unfortunately, our solution could be
         * negative, so we do need to put clamps on our spin/sleep values.
         */
        auto a = vector(1 - m_ratio, 1.);
        auto b = vector(-m_ratio, 1.);
        auto c =
            vector(m_ratio * (m_time.sleep + m_time.run) - m_time.run, quanta);

        auto time_to_spin = std::clamp(c[0] * b[1] - b[0] * c[1], zero, quanta);
        auto time_to_sleep =
            std::clamp(a[0] * c[1] - c[0] * a[1], zero, quanta);

        auto t1 = clock::now();
        clock::time_point t2;
        if (time_to_sleep > zero) {
            std::this_thread::sleep_for(time_to_sleep);
            t2 = clock::now();
            m_time.sleep += t2 - t1;
        } else {
            t2 = t1;
        }

        auto& core_stats = m_result->shard(m_id);
        spin_for(time_to_spin, m_ops, core_stats.targets.data());

        m_time.run += clock::now() - t2;

        auto ut = get_core_utilization_time<clock>(m_id);

        core_stats.last_ = ut.time_stamp;
        core_stats.system = ut.time_system - m_ref.time_system;
        core_stats.user = ut.time_user - m_ref.time_user;
        core_stats.steal = ut.time_steal - m_ref.time_steal;
        core_stats.target = std::chrono::duration_cast<clock::duration>(
            (ut.time_stamp - m_ref.time_stamp) * m_ratio);

        return (0);
    }

    template <typename State> int on_run(const State&)
    {
        /* Nothing to do! */
        return (-1);
    }

    /* State transition functions */
    template <typename State>
    std::optional<state> on_event(const State&, const start_msg& msg)
    {
        m_time.run = clock::duration::zero();
        m_time.sleep = clock::duration::zero();
        m_result = msg.result;

        op_task_sync_ping(m_context, msg.endpoint.c_str());

        OP_LOG(OP_LOG_DEBUG, "Running\n");

        /* Initialize result values */
        m_ref = get_core_utilization_time<clock>(m_id);
        m_result->shard(m_id).last_ = m_ref.time_stamp;
        m_result->shard(m_id).first_ = m_ref.time_stamp;

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
        assert(m_id < msg.values.size());
        OP_LOG(OP_LOG_TRACE,
               "ratio change %.06f -> %.06f\n",
               m_ratio,
               msg.values[m_id] / 100.0);
        m_ratio = msg.values[m_id] / 100.;
        assert(m_ratio <= 1.);

        m_time.run = clock::duration::zero();
        m_time.sleep = clock::duration::zero();

        return (std::nullopt);
    }

    template <typename State, typename Message>
    std::optional<state> on_event(const State&, const Message&)
    {
        return (std::nullopt);
    }
};

int do_work(void* context,
            uint8_t coordinator_id,
            uint8_t core_id,
            const core_config& config,
            const char* endpoint,
            std::promise<void> promise)
{
    op_thread_setname(("op_cpu_" + std::to_string(coordinator_id) + "/"
                       + std::to_string(core_id))
                          .c_str());
    op_thread_set_affinity(core_id);

    std::unique_ptr<void, op_socket_deleter> control(
        op_socket_get_client_subscription(context, endpoint, ""));

    /* Let parent know that our socket is ready */
    promise.set_value();

    auto me = worker(context, config, core_id);

    while (auto cmd = openperf::message::recv(control.get())
                          .and_then(deserialize_command)) {
        me.dispatch(*cmd);
        while (!op_socket_has_messages(control.get())) {
            if (auto error = me.run()) { break; }
        }
    }

    /* Cleanup */
    op_log_close();

    return (0);
}

} // namespace openperf::cpu::generator::worker
