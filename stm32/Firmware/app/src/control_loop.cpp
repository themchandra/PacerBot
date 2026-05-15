/**
 * @file control_loop.cpp
 * @brief Control loop that drives PIDController.
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#include "app/control_loop.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdint.h>

namespace app {

    ControlLoop::ControlLoop(TIM_HandleTypeDef *timer, IMUTask &imu_task,
                             CMDManager &cmd_manager)
        : imu_task_(imu_task), cmd_manager_(cmd_manager)
    {
        esc_.init(timer);
        servo_.init(timer);
        pid_speed_.set_setpoint(SPEED_TEST_SETPOINT_MPS);
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
        // Keep the last values until fresh data arrives.
        float speed_measurement {0.0f};
        float line_position {0.0f};

        // Start timing from the first loop iteration so PID dt matches reality.
        uint32_t last_tick_ms {HAL_GetTick()};

        // Single IMU data buffer reused across iterations to avoid stack churn.
        IMU::Data imu_data {};

        while (isRunning_) {
            // Measure the actual elapsed time for this cycle.
            const uint32_t current_tick_ms {HAL_GetTick()};
            uint32_t elapsed_ms {current_tick_ms - last_tick_ms};
            last_tick_ms = current_tick_ms;

            if (elapsed_ms == 0U) {
                elapsed_ms = 1U;
            }

            // Convert elapsed time to seconds for the PID update.
            const float control_dt_sec = static_cast<float>(elapsed_ms) * 0.001f;

            std::printf("[ctrl] dt=%.3f s tick=%lu\n", control_dt_sec,
                        static_cast<unsigned long>(current_tick_ms));

            // Refresh the line command, otherwise reuse the previous one.
            float line_pos_cmd = 0.0f;
            if (cmd_manager_.get_line_pos(line_pos_cmd)) {
                line_position = line_pos_cmd;
            }

            // Refresh the IMU speed measurement when available. 
            if (imu_task_.get_data(imu_data)) {
                std::printf("[imu ] ax=%.3f ay=%.3f az=%.3f gx=%.3f gy=%.3f gz=%.3f "
                            "vx=%.3f vy=%.3f\n",
                            imu_data.imu.accel_g[0], imu_data.imu.accel_g[1],
                            imu_data.imu.accel_g[2], imu_data.imu.gyro_dps[0],
                            imu_data.imu.gyro_dps[1], imu_data.imu.gyro_dps[2],
                            imu_data.velocity_x_mps, imu_data.velocity_y_mps);
                speed_measurement = imu_data.speed_mps;
            }

            std::printf("[vel ] vx=%.3f vy=%.3f speed=%.3f\n", imu_data.velocity_x_mps,
                        imu_data.velocity_y_mps, speed_measurement);

            // Run both PIDs with the current inputs.
            const float speed_output
                = pid_speed_.update(speed_measurement, control_dt_sec);
            const float line_output = pid_lines_.update(line_position, control_dt_sec);

            std::printf(
                "[pid ] speed_out=%.3f speed_meas=%.3f setpoint=%.3f line_out=%.3f\n",
                speed_output, speed_measurement, pid_speed_.get_setpoint(), line_output);

            // Map normalized PID output to actuator pulse widths.
            const auto esc_us = normalizedToPulse(speed_output, hal::ESC::NEUTRAL_US,
                                                  hal::ESC::MIN_US, hal::ESC::MAX_US);
            const auto servo_us
                = normalizedToPulse(line_output, hal::Servo::CENTER_US,
                                    hal::Servo::MIN_US, hal::Servo::MAX_US);

            std::printf("[out ] esc=%u servo=%u\n", static_cast<unsigned>(esc_us),
                        static_cast<unsigned>(servo_us));

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