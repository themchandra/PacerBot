/**
 * @file pid_controller.h
 * @brief PID controller for control loops (float precision)
 * @author Hayden Mai
 * @date May-13-2026
 */

#ifndef APP_PID_CONTROLLER_H_
#define APP_PID_CONTROLLER_H_

#include <limits>

namespace app {
    /**
     * @class PIDController
     * @brief Simple PID controller with anti-windup and derivative filtering.
     *
     * Usage: configure gains and optional limits, call `set_setpoint()` and
     * then repeatedly call `update(measurement, dt)` to obtain the control
     * output. `dt` is elapsed time in seconds.
     */
    class PIDController {
      public:
        /**
         * @brief Default construct a PIDController with zeroed gains and
         *        unbounded limits.
         */
        PIDController() = default;

        /**
         * @brief Construct with explicit gains.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         */
        PIDController(float kp, float ki, float kd);

        /**
         * @brief Construct with gains and explicit output/integral limits.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         * @param min_out Minimum controller output (clamp).
         * @param max_out Maximum controller output (clamp).
         * @param min_integral Minimum integral accumulator (anti-windup).
         * @param max_integral Maximum integral accumulator (anti-windup).
         */
        PIDController(float kp, float ki, float kd, float min_out, float max_out,
                      float min_integral, float max_integral);

        /**
         * @brief Set the PID gains.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         * @return true on success, false if any gain is non-finite.
         */
        bool set_gains(float kp, float ki, float kd);

        /**
         * @brief Configure the desired target (setpoint) used to compute error.
         * @param setpoint Target value.
         * @return true on success, false if `setpoint` is non-finite.
         * @pre `setpoint` must be finite; passing NaN or infinity results in false.
         * @post Clears internal previous-measurement state so the derivative
         *       term will not produce a spike on the immediately following
         *       `update()` call; derivative resumes after the next sample.
         */
        bool set_setpoint(float setpoint);

        /**
         * @brief Get the currently configured setpoint.
         * @return The setpoint value.
         */
        float get_setpoint() const;

        /**
         * @brief Configure output clamping range.
         * @param min_out Minimum output value.
         * @param max_out Maximum output value.
         * @return true on success, false if inputs are invalid (non-finite or min>max).
         */
        bool set_output_limits(float min_out, float max_out);

        /**
         * @brief Get minimum output clamp.
         * @return Minimum output value.
         */
        float get_min_output() const;

        /**
         * @brief Get maximum output clamp.
         * @return Maximum output value.
         */
        float get_max_output() const;

        /**
         * @brief Configure integral anti-windup limits.
         * @param min_integral Minimum integral accumulator.
         * @param max_integral Maximum integral accumulator.
         * @return true on success, false if inputs are invalid.
         */
        bool set_integral_limits(float min_integral, float max_integral);

        /**
         * @brief Get minimum integral clamp.
         * @return Minimum integral value.
         */
        float get_min_integral() const;

        /**
         * @brief Get maximum integral clamp.
         * @return Maximum integral value.
         */
        float get_max_integral() const;

        /**
         * @brief Set derivative filter time constant (first-order) in seconds.
         *        Set to 0 to disable filtering.
         * @param tau_seconds Time constant in seconds.
         * @return true on success, false if tau is negative or non-finite.
         */
        bool set_derivative_filter(float tau_seconds);

        /**
         * @brief Get configured derivative filter time constant in seconds.
         * @return Time constant (0 means no filtering).
         */
        float get_derivative_filter() const;

        /**
         * @brief Return the last output produced by `update()`.
         * @note Before any `update()` call the value equals clamp(0.0f, min_out,
         * max_out).
         * @return Most recent control output.
         */
        float get_last_output() const;

        /**
         * @brief Reset internal integrator, derivative state and previous sample.
         */
        void reset();

        /**
         * @brief Main update function. Compute control action from the current
         *        `measurement` and elapsed time `dt` (seconds).
         * @param measurement Current sensor measurement.
         * @param dt Elapsed time in seconds since last update.
         * @note Derivative is computed from the measurement (d(measurement)/dt)
         *       and negated so that an increasing measurement reduces the output
         *       when `Kd` is positive. This prevents derivative "kick" on
         *       immediate setpoint changes while remaining equivalent to
         *       d(error)/dt when the setpoint is constant.
         * @return Clamped control output.
         */
        float update(float measurement, float dt);

      private:
        // Gain variables
        float Kp_ {};
        float Ki_ {};
        float Kd_ {};

        // Target value
        float setpoint_ {};
        float min_out_ {std::numeric_limits<float>::lowest()};
        float max_out_ {std::numeric_limits<float>::max()};

        // Previous raw measurement and flag indicating availability
        float prev_measurement_ {};
        bool has_prev_measurement_ {false};

        float integral_ {};
        float min_integral_ {std::numeric_limits<float>::lowest()};
        float max_integral_ {std::numeric_limits<float>::max()};

        float derivative_filter_tau_ {};
        float derivative_ {};
        float last_output_ {};
    };
} // namespace app

#endif
