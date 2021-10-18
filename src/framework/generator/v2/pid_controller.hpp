#ifndef _OP_GENERATOR_PID_CONTROL_HPP_
#define _OP_GENERATOR_PID_CONTROL_HPP_

#include <chrono>
#include <optional>
#include <string>
#include <variant>

#include "core/op_log.h"
#include "utils/overloaded_visitor.hpp"

namespace openperf::generator {

namespace detail {

template <typename T> struct remove_cvref
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template <typename T, typename = void> struct has_value : std::false_type
{};

template <typename T>
struct has_value<T, std::void_t<decltype(T::value)>> : std::true_type
{};

template <typename T>
inline constexpr bool has_value_v = has_value<remove_cvref_t<T>>::value;

template <typename Duration>
std::chrono::duration<double> to_seconds(const Duration& duration)
{
    return (
        std::chrono::duration_cast<std::chrono::duration<double>>(duration));
}

} // namespace detail

template <typename Derived, typename StateVariant, typename EventVariant>
class pid_state_machine
{
    StateVariant m_state;

public:
    std::optional<double> dispatch(EventVariant&& event)
    {
        auto result = std::optional<double>{};

        auto& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](const auto& state,
                auto&& event) -> std::optional<StateVariant> {
                return (child.on_event(state, event));
            },
            m_state,
            std::forward<EventVariant>(event));
        if (next_state) {
            m_state = *next_state;
            result = std::visit(
                [](const auto& state) -> std::optional<double> {
                    if constexpr (detail::has_value_v<decltype(state)>) {
                        return (state.value);
                    } else {
                        return (std::nullopt);
                    }
                },
                m_state);
        }

        return (result);
    }
};

struct pid_state_init
{};

struct pid_state_ready
{
    double value;
};

struct pid_state_control
{};

using pid_state =
    std::variant<pid_state_init, pid_state_ready, pid_state_control>;

struct pid_event_reset
{
    double setpoint;
};

struct pid_event_start
{};

struct pid_event_stop
{
    double value;
};

struct pid_event_update
{
    double setpoint;
};

using pid_event = std::
    variant<pid_event_reset, pid_event_start, pid_event_stop, pid_event_update>;

template <typename Clock>
class pid_controller
    : public pid_state_machine<pid_controller<Clock>, pid_state, pid_event>
{
    friend class pid_state_machine<pid_controller<Clock>, pid_state, pid_event>;
    using parent =
        pid_state_machine<pid_controller<Clock>, pid_state, pid_event>;

    struct setpoints
    {
        double current;
        double min = 0;
        double max = std::numeric_limits<double>::max();
    };

    double Kp_;
    double Ki_;
    double Kd_;
    double Kt_ = 1;
    double beta_ = 1;
    double N_ = 10;
    double integral_ = 0;
    double derivative_ = 0;
    double last_y_ = 0;
    using seconds = std::chrono::duration<double>;
    seconds accumulator_ = seconds::zero();
    struct setpoints setpoint_;
    struct
    {
        typename Clock::time_point start = Clock::time_point::min();
        typename Clock::time_point update = Clock::time_point::min();
    } timestamp_;

protected:
    template <typename State>
    std::optional<pid_state> on_event(const State&,
                                      const pid_event_reset& event)
    {
        integral_ = 0;
        derivative_ = 0;
        accumulator_ = seconds::zero();
        last_y_ = event.setpoint;
        setpoint_.current = event.setpoint;
        timestamp_.start = Clock::time_point::min();
        timestamp_.update = Clock::time_point::min();

        return (pid_state_ready{0});
    }

    std::optional<pid_state> on_event(const pid_state_ready&,
                                      const pid_event_start&)
    {
        accumulator_ = seconds::zero();
        timestamp_.start = Clock::now();
        timestamp_.update = timestamp_.start;

        return (pid_state_control{});
    }

    std::optional<pid_state> on_event(const pid_state_control&,
                                      const pid_event_stop& stop)
    {
        /*
         * These PID control calculations are based on the pseudocode found in
         * Chapter 10 of Feedback Systems: An Introduction for Scientists and
         * Engineers by Astrom and Murray.
         */
        auto now = Clock::now();
        accumulator_ += (now - timestamp_.update) * setpoint_.current;
        auto dt = detail::to_seconds(now - timestamp_.start);
        auto Tf = Kp_ ? (Kd_ / Kp_) / N_ : 0;
        auto bi = Ki_ * dt.count();
        auto ad = Tf / (Tf + dt.count());
        auto bd = Kd_ / (Tf + dt.count());

        auto P = Kp_ * (beta_ * accumulator_.count() - stop.value);
        derivative_ = ad * derivative_ - bd * (stop.value - last_y_);
        auto v = P + derivative_ + integral_;
        auto u = saturate(setpoint_, v);
        integral_ = integral_ + bi * (accumulator_.count() - stop.value)
                    + Kt_ * (u - v);
        last_y_ = stop.value;

        // OP_LOG(OP_LOG_TRACE,
        //       "%s: %.06f,%.06f,%.06f,%.06f,%.06f,%.06f,%.06f\n ",
        //       __func__,
        //       accumulator_.count(),
        //       stop.value,
        //       u,
        //       v,
        //       P,
        //       integral_,
        //       derivative_);

        return (pid_state_ready{u});
    }

    std::optional<pid_state> on_event(const pid_state_control&,
                                      const pid_event_update& event)
    {
        auto now = Clock::now();
        accumulator_ += (now - timestamp_.update) * setpoint_.current;
        timestamp_.update = now;
        last_y_ = event.setpoint;
        setpoint_.current = event.setpoint;

        return (std::nullopt);
    }

    template <typename State, typename Event>
    std::optional<pid_state> on_event(const State&, const Event&)
    {
        OP_LOG(OP_LOG_WARNING,
               "somebody called useless pid_controller::on_event()\n");
        return (std::nullopt);
    }

    inline static double saturate(const setpoints& s, double v)
    {
        if (s.current + v > s.max) { return (s.max - s.current); }

        if (s.current + v < s.min) { return (s.min - s.current); }

        return (v);
    }

public:
    pid_controller(double Kp, double Ki, double Kd)
        : Kp_(Kp)
        , Ki_(Ki)
        , Kd_(Kd)
    {}

    void max(double x) { setpoint_.max = x; }

    void reset(double setpoint) { parent::dispatch(pid_event_reset{setpoint}); }

    void start() { parent::dispatch(pid_event_start{}); }

    double stop(double actual)
    {
        return (parent::dispatch(pid_event_stop{actual}).value());
    }

    void update(double setpoint)
    {
        parent::dispatch(pid_event_update{setpoint});
    }
};

} // namespace openperf::generator

#endif /* _OP_GENERATOR_PID_CONTROL_HPP_ */
