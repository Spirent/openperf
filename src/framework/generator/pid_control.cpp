#include "pid_control.hpp"

#include <cassert>

namespace openperf::framework::generator {

// Constructors & Destructor
pid_control::pid_control(double Kp, double Ki, double Kd)
    : m_kp(Kp)
    , m_ki(Ki)
    , m_kd(Kd)
{}

// Methods : public
void pid_control::reset(double setpoint)
{
    m_integral = 0;
    m_derivative = 0;
    m_accumulator = 0;
    m_last_y = m_setpoint = setpoint;
    m_start_ts = m_update_ts = chronometer::now();
    m_state = state_t::READY;
}

void pid_control::start()
{
    assert(m_state == state_t::READY);
    m_accumulator = 0.0;
    m_start_ts = m_update_ts = chronometer::now();
    m_state = state_t::CONTROL;
}
/*
 * These PID control calculations are based on the pseudocode found in
 * Chapter 10 of Feedback Systems: An Introduction for Scientists and Engineers
 * by Astrom and Murray.
 */
double pid_control::stop(double y)
{
    assert(m_state == state_t::CONTROL);
    auto now = chronometer::now();
    m_accumulator += static_cast<double>((now - m_update_ts).count())
                     / std::nano::den * m_setpoint;
    double dtime =
        static_cast<double>((now - m_start_ts).count()) / std::nano::den;
    double Tf = m_kp ? (m_kd / m_kp) / m_n : 0.0;
    double bi = m_ki * dtime;
    double ad = Tf / (Tf + dtime);
    double bd = m_kd / (Tf + dtime);

    double P = m_kp * (m_beta * m_accumulator - y);
    m_derivative = ad * m_derivative - bd * (y - m_last_y);
    double v = P + m_derivative + m_integral;
    double u = saturate(v);
    m_integral = m_integral + bi * (m_accumulator - y) + m_kt * (u - v);
    m_last_y = y;

    // OP_LOG(OP_LOG_INFO, "%s:
    // %.06f\t%.06f\t%.06f\t%.06f\t%.06f\t%.06f\t%.06f\n",
    //        __func__,
    //        m_accumulator, y, u, v, P, m_integral, m_derivative);

    m_state = state_t::READY;
    return u;
}

void pid_control::update(double setpoint)
{
    if (m_state == state_t::CONTROL) {
        auto now = chronometer::now();
        m_accumulator += static_cast<double>((now - m_update_ts).count())
                         / std::nano::den * m_setpoint;
        m_update_ts = now;
        m_last_y = m_setpoint = setpoint;
    }
}

// Methods : private
double pid_control::saturate(double v)
{
    if (m_setpoint + v > m_setpoint_max) return m_setpoint_max - m_setpoint;
    if (m_setpoint + v < m_setpoint_min) return m_setpoint_min - m_setpoint;
    return v;
}

} // namespace openperf::framework::generator
