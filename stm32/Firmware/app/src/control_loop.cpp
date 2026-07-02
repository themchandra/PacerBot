/**
 * @file control_loop.cpp
 * @brief Control loop that drives PIDController.
 * @author Hayden Mai
 * @date Jul-02-2026
 */

#include "app/control_loop.h"

#include "comm/uart/callbacks.h"

#include <algorithm>
#include <cmath>
#include <stdint.h>

namespace app {

    ControlLoop::ControlLoop(TIM_HandleTypeDef *timer, UART_HandleTypeDef *huart_gps)
        : gps_(huart_gps)
    {
        mutex_ = osMutexNew(nullptr);

        uart::callbacks::set_gps(&gps_);

        esc_.init(timer);
        servo_.init(timer);
        pid_speed_.set_setpoint(target_speed_mps_);
        pid_lines_.set_setpoint(TARGET_POSITION);

        // Apply configured PID gains
        pid_speed_.set_gains(KP_SPEED, KI_SPEED, KD_SPEED);
        pid_lines_.set_gains(KP_LINE, KI_LINE, KD_LINE);

        // Keep controller outputs normalized to [-1.0, 1.0]
        // Actuator mapping happens in normalizedToPulse().
        pid_speed_.set_output_limits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);
        pid_speed_.set_integral_limits(-calculateIntegralLimits(KI_SPEED),
                                       calculateIntegralLimits(KI_SPEED));

        pid_lines_.set_output_limits(PID_OUTPUT_MIN, PID_OUTPUT_MAX);
        pid_lines_.set_integral_limits(-calculateIntegralLimits(KI_LINE),
                                       calculateIntegralLimits(KI_LINE));
    }


    void ControlLoop::start()
    {
        if (taskHandle_ != nullptr) {
            return;
        }
        gps_.start();

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

        osMutexAcquire(mutex_, osWaitForever);
        pid_speed_.reset();
        pid_lines_.reset();
        measured_line_position_ = 0.0f;
        target_speed_mps_       = 0.0f;
        osMutexRelease(mutex_);

        esc_.set_pulse_us(hal::ESC::NEUTRAL_US);
        servo_.set_pulse_us(hal::Servo::CENTER_US);
    }


    void ControlLoop::set_target_speed(float speed_mps)
    {
        osMutexAcquire(mutex_, osWaitForever);
        target_speed_mps_
            = std::clamp(speed_mps, TARGET_SPEED_MIN_MPS, TARGET_SPEED_MAX_MPS);
        osMutexRelease(mutex_);
    }


    void ControlLoop::set_measured_line_position(float position)
    {
        osMutexAcquire(mutex_, osWaitForever);
        measured_line_position_ = position;
        osMutexRelease(mutex_);
    }


    void ControlLoop::threadTrampoline(void *args)
    {
        ControlLoop *pThis = static_cast<ControlLoop *>(args);
        pThis->threadLoop();
    }


    void ControlLoop::threadLoop()
    {
        // Wait for a 3D satellite fix before starting the control loop
        hal::GPS::Data gps_data = gps_.getData();
        while (!gps_data.valid || gps_data.fix_qual < 3) {
            osDelay(1000);
            gps_data = gps_.getData();
        }

        uint32_t prev_tick_ms {HAL_GetTick()};

        while (isRunning_) {
            const uint32_t cur_tick_ms {HAL_GetTick()};
            uint32_t elapsed_ms {cur_tick_ms - prev_tick_ms};
            prev_tick_ms = cur_tick_ms;

            if (elapsed_ms == 0U) {
                elapsed_ms = 1U;
            }

            const hal::GPS::Data gps_data = gps_.getData();

            osMutexAcquire(mutex_, osWaitForever);
            const float measured_line_position = measured_line_position_;
            const float target_speed           = target_speed_mps_;
            osMutexRelease(mutex_);

            if (pid_speed_.get_setpoint() != target_speed) {
                pid_speed_.set_setpoint(target_speed);
            }

            const float speed_output = pid_speed_.update(gps_data.speed_mps, elapsed_ms);
            const float line_output
                = pid_lines_.update(measured_line_position, elapsed_ms);

            // Set PID outputs to actuators
            const auto esc_us = normalizedToPulse(speed_output, hal::ESC::NEUTRAL_US,
                                                  hal::ESC::MIN_US, hal::ESC::MAX_US);
            const auto servo_us
                = normalizedToPulse(line_output, hal::Servo::CENTER_US,
                                    hal::Servo::MIN_US, hal::Servo::MAX_US);
            esc_.set_pulse_us(esc_us);
            servo_.set_pulse_us(servo_us);

            osDelay(CONTROL_PERIOD_MS);
        }
    }


    float ControlLoop::calculateIntegralLimits(float ki)
    {
        if (ki == 0.0f) {
            return PID_OUTPUT_MAX;
        }

        return PID_OUTPUT_MAX / std::fabs(ki);
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