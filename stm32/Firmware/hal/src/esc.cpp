/**
 * @file esc.cpp
 * @brief ESC control using TIM3 PWM channel 1.
 */

#include "hal/esc.h"

namespace hal {

    bool ESC::init(TIM_HandleTypeDef *timer) { return pwm_.init(timer); }


    void ESC::set_pulse_us(uint16_t pulse_us) { pwm_.setPulseUs(pulse_us); }

} // namespace hal