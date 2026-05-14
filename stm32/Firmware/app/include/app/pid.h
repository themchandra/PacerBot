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
    class PIDController {
      public:
        PIDController() = default;
        PIDController(double kp, double ki, double kd);

        void set_gains(double kp, double ki, double kd);
        void set_setpoint(double setpoint);
        double get_setpoint() const;

        bool set_output_limits(double min_out, double max_out);
        double get_min_output() const;
        double get_max_output() const;

        bool set_integral_limits(double min_integral, double max_integral);
        double get_min_integral() const;
        double get_max_integral() const;

        bool set_derivative_filter(double tau_seconds);
        double get_derivative_filter() const;
        double get_last_output() const;

        void reset();

        double update(double measurement, double dt);

      private:
        //
        double Kp_ {};
        double Ki_ {};
        double Kd_ {};

        // Target value
        double setpoint_ {};
        double min_out_ {std::numeric_limits<double>::lowest()};
        double max_out_ {std::numeric_limits<double>::max()};

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