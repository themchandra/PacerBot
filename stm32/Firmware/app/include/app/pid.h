/**
 * @file pid.h
 * @brief Simple reusable PID controller for firmware control loops.
 */

#ifndef APP_PID_H_
#define APP_PID_H_

namespace app {

    class PID {
      public:
        PID(float kp = 0.0f, float ki = 0.0f, float kd = 0.0f);

        void setTunings(float kp, float ki, float kd);
        void setOutputLimits(float min_output, float max_output);
        void setSetpoint(float setpoint);
        void reset();

        float update(float measurement, float dt_seconds);

        float getSetpoint() const;
        float getKp() const;
        float getKi() const;
        float getKd() const;

      private:
        float kp_;
        float ki_;
        float kd_;

        float setpoint_;
        float integral_;
        float previous_measurement_;
        float last_output_;
        bool first_update_;

        float min_output_;
        float max_output_;

        float clamp(float value) const;
    };

} // namespace app

#endif // APP_PID_H_
