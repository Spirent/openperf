#ifndef _OP_FRAMEWORK_PID_CONTROLLER_HPP_
#define _OP_FRAMEWORK_PID_CONTROLLER_HPP_

#include <cinttypes>
#include <chrono>

namespace openperf::framework::generator {

class pid_control
{
private:
    using chronometer = std::chrono::steady_clock;
    enum class state_t : uint8_t {
        NONE = 0,
        INIT,
        READY,
        CONTROL,
    };

private:
    double m_kt = 1.0;
    double m_n = 10.0;
    double m_beta = 1.0;
    double m_setpoint_max = __DBL_MAX__;
    double m_setpoint_min = 0.0;
    chronometer::time_point m_start_ts = chronometer::now();
    chronometer::time_point m_update_ts = chronometer::now();
    state_t m_state = state_t::INIT;

    double m_kp;
    double m_ki;
    double m_kd;

    double m_accumulator;
    double m_integral;
    double m_derivative;
    double m_last_y;
    double m_setpoint;

public:
    pid_control(double Kp, double Ki, double Kd);

    double min() const { return m_setpoint_min; }
    double max() const { return m_setpoint_max; }
    double n() const { return m_n; }
    double kt() const { return m_kt; }
    double beta() const { return m_beta; }

    void min(double m) { m_setpoint_min = m; }
    void max(double m) { m_setpoint_max = m; }
    void n(double n) { m_n = n; }
    void kt(double kt) { m_kt = kt; }
    void beta(double beta) { m_beta = beta; }

    void start();
    double stop(double);
    void update(double);
    void reset(double);

private:
    double saturate(double);
};

} // namespace openperf::framework::generator

#endif // _OP_FRAMEWORK_PID_CONTROLLER_HPP_
