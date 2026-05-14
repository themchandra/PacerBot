/**
 * @file pwm.h
 * @brief PWM output wrapper
 * @author Hayden Mai
 * @date May-13-2026
 */

#include "hal/pwm.h"

namespace hal {

    PWM::PWM(uint32_t channel, uint16_t min_us, uint16_t max_us, uint16_t neutral_us)
        : channel_ {channel}, min_us_ {min_us}, max_us_ {max_us}, neutral_us_ {neutral_us}
    {}


    uint16_t PWM::clampPulse(uint16_t pulse_us) const
    {
        if (pulse_us < min_us_) {
            return min_us_;
        }

        if (pulse_us > max_us_) {
            return max_us_;
        }

        return pulse_us;
    }


    bool PWM::init(TIM_HandleTypeDef *timer)
    {
        if (timer == nullptr) {
            return false;
        }

        timer_ = timer;

        if (HAL_TIM_PWM_Start(timer_, channel_) != HAL_OK) {
            return false;
        }

        setPulseUs(neutral_us_);
        return true;
    }


    void PWM::setPulseUs(uint16_t pulse_us)
    {
        if (timer_ == nullptr) {
            return;
        }

        const uint16_t safe_pulse_us = clampPulse(pulse_us);
        __HAL_TIM_SET_COMPARE(timer_, channel_, safe_pulse_us);
    }

} // namespace hal