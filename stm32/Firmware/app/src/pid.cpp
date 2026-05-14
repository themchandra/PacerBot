/**
 * @file pid.cpp
 * @brief A PID class for control loops
 * @author Hayden Mai
 * @date May-13-2026
 */

#include "app/pid.h"

#include <algorithm>
#include <cmath>

namespace {
    bool isFinite(double value) { return std::isfinite(value); }


    double clampValue(double value, double minValue, double maxValue)
    {
        return std::clamp(value, minValue, maxValue);
    }
} // namespace


namespace app {

    PIDController::PIDController(double kp, double ki, double kd)
    {
        set_gains(kp, ki, kd);
        last_output_ = clampValue(0.0, min_out_, max_out_);
    }


    PIDController::PIDController(double kp, double ki, double kd, double min_out,
                                 double max_out, double min_integral, double max_integral)
        : PIDController(kp, ki, kd)
    {
        set_output_limits(min_out, max_out);
        set_integral_limits(min_integral, max_integral);
        last_output_ = clampValue(last_output_, min_out_, max_out_);
    }


    bool PIDController::set_gains(double kp, double ki, double kd)
    {
        // Validate finite gains
        if (!isFinite(kp) || !isFinite(ki) || !isFinite(kd)) {
            return false;
        }

        Kp_ = kp;
        Ki_ = ki;
        Kd_ = kd;
        return true;
    }


    bool PIDController::set_setpoint(double setpoint)
    {
        if (!isFinite(setpoint)) {
            return false;
        }

        // Update setpoint and clear previous-measurement state so the
        // derivative term does not produce a large spike immediately after
        // changing the setpoint. The controller will resume derivative
        // calculations after the next `update()` call provides a fresh sample.
        setpoint_             = setpoint;
        has_prev_measurement_ = false;
        prev_measurement_     = 0.0;
        return true;
    }


    double PIDController::get_setpoint() const { return setpoint_; }


    bool PIDController::set_output_limits(double min_out, double max_out)
    {
        if (!isFinite(min_out) || !isFinite(max_out) || min_out > max_out) {
            return false;
        }

        min_out_     = min_out;
        max_out_     = max_out;
        last_output_ = clampValue(last_output_, min_out_, max_out_);
        return true;
    }


    double PIDController::get_min_output() const { return min_out_; }


    double PIDController::get_max_output() const { return max_out_; }


    bool PIDController::set_integral_limits(double min_integral, double max_integral)
    {
        if (!isFinite(min_integral) || !isFinite(max_integral)
            || min_integral > max_integral) {
            return false;
        }

        min_integral_ = min_integral;
        max_integral_ = max_integral;
        integral_     = clampValue(integral_, min_integral_, max_integral_);
        return true;
    }


    double PIDController::get_min_integral() const { return min_integral_; }


    double PIDController::get_max_integral() const { return max_integral_; }


    bool PIDController::set_derivative_filter(double tau_seconds)
    {
        if (!isFinite(tau_seconds) || tau_seconds < 0.0) {
            return false;
        }

        derivative_filter_tau_ = tau_seconds;
        return true;
    }


    double PIDController::get_derivative_filter() const { return derivative_filter_tau_; }


    double PIDController::get_last_output() const { return last_output_; }


    void PIDController::reset()
    {
        integral_             = 0.0;
        derivative_           = 0.0;
        prev_measurement_     = 0.0;
        has_prev_measurement_ = false;
        last_output_          = clampValue(0.0, min_out_, max_out_);
    }


    double PIDController::update(double measurement, double dt)
    {
        if (!isFinite(measurement) || !isFinite(dt) || dt <= 0.0) {
            return last_output_;
        }

        // error = setpoint - measurement
        // (positive error means measurement is below target)
        const double error = setpoint_ - measurement;

        // Integrate error: integral += error * dt
        // Clamp to configured integral limits to prevent windup
        integral_ = clampValue(integral_ + (error * dt), min_integral_, max_integral_);

        double rawDerivative = 0.0;
        if (has_prev_measurement_) {
            // Derivative on measurement: d(measurement)/dt approximated by
            // (measurement - prev_measurement)/dt. To produce the same sign
            // convention as d(error)/dt we negate the measurement derivative
            // so that an increasing measurement reduces the output when Kd>0.
            rawDerivative = -(measurement - prev_measurement_) / dt;
        }

        if (derivative_filter_tau_ > 0.0) {
            const double alpha = dt / (derivative_filter_tau_ + dt);
            // First-order low-pass filter on derivative:
            // derivative = derivative + alpha * (raw - derivative)
            // where alpha = dt / (tau + dt)
            derivative_ += alpha * (rawDerivative - derivative_);
        } else {
            derivative_ = rawDerivative;
        }

        // PID output = Kp*error + Ki*integral + Kd*derivative
        const double output = (Kp_ * error) + (Ki_ * integral_) + (Kd_ * derivative_);
        last_output_        = clampValue(output, min_out_, max_out_);

        prev_measurement_     = measurement;
        has_prev_measurement_ = true;

        return last_output_;
    }

} // namespace app