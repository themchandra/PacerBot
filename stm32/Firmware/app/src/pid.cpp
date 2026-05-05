/**
 * @file pid.cpp
 * @brief Simple reusable PID controller for firmware control loops.
 */

#include <limits>

#include "app/pid.h"

namespace app {

    PID::PID(float kp, float ki, float kd)
        : kp_(kp),
          ki_(ki),
          kd_(kd),
          setpoint_(0.0f),
          integral_(0.0f),
          previous_measurement_(0.0f),
          last_output_(0.0f),
          first_update_(true),
          min_output_(-std::numeric_limits<float>::infinity()),
          max_output_(std::numeric_limits<float>::infinity())
    {}

    void PID::setTunings(float kp, float ki, float kd)
    {
        kp_ = kp;
        ki_ = ki;
        kd_ = kd;
    }

    void PID::setOutputLimits(float min_output, float max_output)
    {
        min_output_ = min_output;
        max_output_ = max_output;

        if (min_output_ > max_output_) {
            const float swap = min_output_;
            min_output_      = max_output_;
            max_output_      = swap;
        }
        last_output_ = clamp(last_output_);
    }

    void PID::setSetpoint(float setpoint) { setpoint_ = setpoint; }

    void PID::reset()
    {
        integral_             = 0.0f;
        previous_measurement_ = 0.0f;
        last_output_          = 0.0f;
        first_update_         = true;
    }

    float PID::update(float measurement, float dt_seconds)
    {
        if (dt_seconds <= 0.0f) {
            return last_output_;
        }

        const float error = setpoint_ - measurement;

        integral_ += error * dt_seconds;

        if (ki_ != 0.0f) {
            const float min_integral = min_output_ / ki_;
            const float max_integral = max_output_ / ki_;

            if (min_integral < max_integral) {
                if (integral_ < min_integral) {
                    integral_ = min_integral;
                } else if (integral_ > max_integral) {
                    integral_ = max_integral;
                }
            } else {
                if (integral_ < max_integral) {
                    integral_ = max_integral;
                } else if (integral_ > min_integral) {
                    integral_ = min_integral;
                }
            }
        }

        const float derivative
            = first_update_ ? 0.0f : -(measurement - previous_measurement_) / dt_seconds;

        first_update_         = false;
        previous_measurement_ = measurement;

        const float output = (kp_ * error) + (ki_ * integral_) + (kd_ * derivative);

        last_output_ = clamp(output);
        return last_output_;
    }

    float PID::getSetpoint() const { return setpoint_; }

    float PID::getKp() const { return kp_; }

    float PID::getKi() const { return ki_; }

    float PID::getKd() const { return kd_; }

    float PID::clamp(float value) const
    {
        if (min_output_ > max_output_) {
            return value;
        }

        if (value < min_output_) {
            return min_output_;
        }

        if (value > max_output_) {
            return max_output_;
        }

        return value;
    }

} // namespace app
