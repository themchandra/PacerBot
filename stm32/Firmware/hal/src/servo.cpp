/**
 * @file servo.cpp
 * @brief Servo control using TIM3 PWM channel 2.
 */

#include "hal/servo.h"

#include "main.h"

namespace hal {

    uint16_t Servo::clampPulse(uint16_t pulseUs)
    {
        if (pulseUs < SERVO_MIN_PULSE_US) {
            return SERVO_MIN_PULSE_US;
        }

        if (pulseUs > SERVO_MAX_PULSE_US) {
            return SERVO_MAX_PULSE_US;
        }

        return pulseUs;
    }

    bool Servo::init(TIM_HandleTypeDef *timer)
    {
        if (timer == nullptr) {
            return false;
        }

        servoTimer_ = timer;

        if (HAL_TIM_PWM_Start(servoTimer_, SERVO_CHANNEL) != HAL_OK) {
            return false;
        }

        setPulseUs(SERVO_CENTER_PULSE_US);
        return true;
    }

    void Servo::setPulseUs(uint16_t pulseUs)
    {
        if (servoTimer_ == nullptr) {
            return;
        }

        const uint16_t safePulseUs = clampPulse(pulseUs);
        __HAL_TIM_SET_COMPARE(servoTimer_, SERVO_CHANNEL, safePulseUs);
    }

} // namespace hal