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
        static constexpr float CONTROL_PERIOD_SEC {0.01f};

        float speed_measurement = 0.0f;
        float line_position     = 0.0f;

        while (isRunning_) {
            float target_speed = 0.0f;
            if (cmd_manager_.get_target_speed(target_speed)
                && std::isfinite(target_speed)) {
                pid_speed_.set_setpoint(target_speed);
            }

            float line_pos_cmd = 0.0f;
            if (cmd_manager_.get_line_pos(line_pos_cmd) && std::isfinite(line_pos_cmd)) {
                line_position = line_pos_cmd;
            }

            IMU::Data imu_data {};
            if (imu_task_.get_data(imu_data) && std::isfinite(imu_data.accel_g[0])) {
                speed_measurement = imu_data.accel_g[0];
            }

            const float speed_output
                = pid_speed_.update(speed_measurement, CONTROL_PERIOD_SEC);
            const float line_output
                = pid_lines_.update(line_position, CONTROL_PERIOD_SEC);

            const float esc_target
                = static_cast<float>(hal::ESC::NEUTRAL_US) + speed_output;
            const float servo_target
                = static_cast<float>(hal::Servo::CENTER_US) + line_output;

            const auto esc_us = static_cast<uint16_t>(
                std::clamp(esc_target, static_cast<float>(hal::ESC::MIN_US),
                           static_cast<float>(hal::ESC::MAX_US)));
            const auto servo_us = static_cast<uint16_t>(
                std::clamp(servo_target, static_cast<float>(hal::Servo::MIN_US),
                           static_cast<float>(hal::Servo::MAX_US)));

            esc_.set_pulse_us(esc_us);
            servo_.set_pulse_us(servo_us);

            osDelay(CONTROL_PERIOD_MS);
        }
    }

} // namespace app