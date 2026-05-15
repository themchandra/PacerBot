/**
 * @file control_loop.cpp
 * @brief Control loop that drives PIDController.
 * @author Hayden Mai
 * @date May-14-2026
 */

#include "app/control_loop.h"

#include <algorithm>
#include <cmath>
#include <stdint.h>

namespace app {

    ControlLoop::ControlLoop(TIM_HandleTypeDef *timer, IMUTask &imu_task,
                             CMDManager &cmd_manager)
        : imu_task_(imu_task), cmd_manager_(cmd_manager)
    {
        esc_.init(timer);
        servo_.init(timer);
        pid_lines_.set_setpoint(0.0f);

        // Apply configured PID gains
        pid_speed_.set_gains(KP_SPEED, KI_SPEED, KD_SPEED);
        pid_lines_.set_gains(KP_LINE, KI_LINE, KD_LINE);

        // Keep controller outputs normalized; actuator mapping happens later.
        pid_speed_.set_output_limits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);
        pid_speed_.set_integral_limits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);

        pid_lines_.set_output_limits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);
        pid_lines_.set_integral_limits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);
    }


    void ControlLoop::start()
    {
        if (taskHandle_ != nullptr) {
            return;
        }

        isRunning_  = true;
        taskHandle_ = osThreadNew(threadTrampoline, this, &task_att_);
        if (taskHandle_ == nullptr) {
            isRunning_ = false;
        }
    }


    void ControlLoop::stop()
    {
        isRunning_ = false;

        if (taskHandle_ != nullptr) {
            osThreadTerminate(taskHandle_);
            taskHandle_ = nullptr;
        }

        pid_speed_.reset();
        pid_lines_.reset();
        esc_.set_pulse_us(hal::ESC::NEUTRAL_US);
        servo_.set_pulse_us(hal::Servo::CENTER_US);
    }


    void ControlLoop::threadTrampoline(void *args)
    {
        ControlLoop *pThis = static_cast<ControlLoop *>(args);
        pThis->threadLoop();
    }


    void ControlLoop::threadLoop()
    {
        static constexpr uint32_t CONTROL_PERIOD_MS {10};

        // Keep the last values until fresh data arrives.
        float speed_measurement {0.0f};
        float line_position {0.0f};
        // Start timing from the first loop iteration.
        uint32_t last_tick_ms {HAL_GetTick()};

        while (isRunning_) {
            // Measure the actual elapsed time for this cycle.
            const uint32_t current_tick_ms {HAL_GetTick()};
            uint32_t elapsed_ms {current_tick_ms - last_tick_ms};
            last_tick_ms = current_tick_ms;

            // Prevent a zero dt from millisecond tick resolution.
            if (elapsed_ms == 0U) {
                elapsed_ms = 1U;
            }

            // Convert elapsed time to seconds for the PID update.
            const float control_dt_sec = static_cast<float>(elapsed_ms) * 0.001f;

            // Update the speed setpoint when a new command arrives.
            float target_speed = 0.0f;
            if (cmd_manager_.get_target_speed(target_speed)) {
                pid_speed_.set_setpoint(target_speed);
            }

            // Refresh the line command, otherwise reuse the previous one.
            float line_pos_cmd = 0.0f;
            if (cmd_manager_.get_line_pos(line_pos_cmd)) {
                line_position = line_pos_cmd;
            }

            // Refresh the IMU speed measurement when available.
            IMU::Data imu_data {};
            if (imu_task_.get_data(imu_data)) {
                speed_measurement = imu_data.accel_g[0];
            }

            // Run both PIDs with the current inputs.
            const float speed_output
                = pid_speed_.update(speed_measurement, control_dt_sec);
            const float line_output = pid_lines_.update(line_position, control_dt_sec);

            // Map normalized PID output to actuator pulse widths.
            const auto esc_us = normalizedToPulse(speed_output, hal::ESC::NEUTRAL_US,
                                                  hal::ESC::MIN_US, hal::ESC::MAX_US);
            const auto servo_us
                = normalizedToPulse(line_output, hal::Servo::CENTER_US,
                                    hal::Servo::MIN_US, hal::Servo::MAX_US);

            // Send the new ESC and servo commands.
            esc_.set_pulse_us(esc_us);
            servo_.set_pulse_us(servo_us);

            // Wait for the next cycle.
            osDelay(CONTROL_PERIOD_MS);
        }
    }


    uint16_t ControlLoop::normalizedToPulse(float normalized, uint16_t center_us,
                                            uint16_t min_us, uint16_t max_us)
    {
        if (normalized >= 0.0f) {
            const float pulse = static_cast<float>(center_us)
                              + (normalized * (static_cast<float>(max_us) - center_us));
            return static_cast<uint16_t>(std::lround(pulse));
        }

        const float pulse = static_cast<float>(center_us)
                          + (normalized * (center_us - static_cast<float>(min_us)));
        return static_cast<uint16_t>(std::lround(pulse));
    }

} // namespace app