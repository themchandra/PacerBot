/**
 * @file pid.h
 * @brief A PID class for control loops
 * @author Hayden Mai
 * @date May-13-2026
 */

#ifndef APP_PID_H_
#define APP_PID_H_

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
        PIDController(double kp, double ki, double kd);

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
        PIDController(double kp, double ki, double kd, double min_out, double max_out,
                      double min_integral, double max_integral);

        /**
         * @brief Set the PID gains.
         * @param kp Proportional gain.
         * @param ki Integral gain.
         * @param kd Derivative gain.
         * @return true on success, false if any gain is non-finite.
         */
        bool set_gains(double kp, double ki, double kd);

        /**
         * @brief Configure the desired target (setpoint) used to compute error.
         * @param setpoint Target value.
         * @return true on success, false if `setpoint` is non-finite.
         * @pre `setpoint` must be finite; passing NaN or infinity results in false.
         * @post Clears internal previous-measurement state so the derivative
         *       term will not produce a spike on the immediately following
         *       `update()` call; derivative resumes after the next sample.
         */
        bool set_setpoint(double setpoint);

        /**
         * @brief Get the currently configured setpoint.
         * @return The setpoint value.
         */
        double get_setpoint() const;

        /**
         * @brief Configure output clamping range.
         * @param min_out Minimum output value.
         * @param max_out Maximum output value.
         * @return true on success, false if inputs are invalid (non-finite or min>max).
         */
        bool set_output_limits(double min_out, double max_out);

        /**
         * @brief Get minimum output clamp.
         * @return Minimum output value.
         */
        double get_min_output() const;

        /**
         * @brief Get maximum output clamp.
         * @return Maximum output value.
         */
        double get_max_output() const;

        /**
         * @brief Configure integral anti-windup limits.
         * @param min_integral Minimum integral accumulator.
         * @param max_integral Maximum integral accumulator.
         * @return true on success, false if inputs are invalid.
         */
        bool set_integral_limits(double min_integral, double max_integral);

        /**
         * @brief Get minimum integral clamp.
         * @return Minimum integral value.
         */
        double get_min_integral() const;

        /**
         * @brief Get maximum integral clamp.
         * @return Maximum integral value.
         */
        double get_max_integral() const;

        /**
         * @brief Set derivative filter time constant (first-order) in seconds.
         *        Set to 0 to disable filtering.
         * @param tau_seconds Time constant in seconds.
         * @return true on success, false if tau is negative or non-finite.
         */
        bool set_derivative_filter(double tau_seconds);

        /**
         * @brief Get configured derivative filter time constant in seconds.
         * @return Time constant (0 means no filtering).
         */
        double get_derivative_filter() const;

        /**
         * @brief Return the last output produced by `update()`.
         * @note Before any `update()` call the value equals clamp(0.0, min_out, max_out).
         * @return Most recent control output.
         */
        double get_last_output() const;

        /**
         * @brief Reset internal integrator, derivative state and previous sample.
         */
        void reset();

        /**
         * @brief Main update function. Compute control action from the current
         *        `measurement` and elapsed time `dt` (seconds).
         * @param measurement Current sensor measurement.
         * @param dt Elapsed time in seconds since last update.
         * @note The derivative term is computed on the measurement
         *       (d(measurement)/dt) and applied with a negative sign to avoid
         *       producing spikes when the setpoint changes. This is equivalent
         *       to computing d(error)/dt when the setpoint is constant but
         *       prevents derivative 'kick' on setpoint steps.
         * @return Clamped control output.
         */
        double update(double measurement, double dt);

      private:
        // Gain variables
        double Kp_ {};
        double Ki_ {};
        double Kd_ {};

        // Target value
        double setpoint_ {};
        double min_out_ {std::numeric_limits<double>::lowest()};
        double max_out_ {std::numeric_limits<double>::max()};

        // Previous raw measurement and flag indicating availability
        double prev_measurement_ {};
        bool has_prev_measurement_ {false};

        double integral_ {};
        double min_integral_ {std::numeric_limits<double>::lowest()};
        double max_integral_ {std::numeric_limits<double>::max()};

        double derivative_filter_tau_ {};
        double derivative_ {};
        double last_output_ {};
    };
} // namespace app

#endif