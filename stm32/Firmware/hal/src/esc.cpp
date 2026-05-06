/**
 * @file esc.cpp
 * @brief ESC control using TIM3 PWM channel 1.
 */

#include "hal/esc.h"

#include "main.h"

extern "C" {
extern TIM_HandleTypeDef htim3;
}

namespace hal {

    uint16_t ESC::clampPulse(uint16_t pulseUs)
    {
        if (pulseUs < MIN_US) {
            return MIN_US;
        }

        if (pulseUs > MAX_US) {
            return MAX_US;
        }

        return pulseUs;
    }

    void ESC::init()
    {
        timer_ = &htim3;

        if (HAL_TIM_PWM_Start(timer_, ESC_CHANNEL) == HAL_OK) {
            set_pulse_us(NEUTRAL_US);
        }
    }

    void ESC::set_pulse_us(uint16_t pulseUs)
    {
        if (timer_ == nullptr) {
            return;
        }

        const uint16_t safePulseUs = clampPulse(pulseUs);
        __HAL_TIM_SET_COMPARE(timer_, ESC_CHANNEL, safePulseUs);
    }

} // namespace hal