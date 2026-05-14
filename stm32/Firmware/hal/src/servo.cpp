/**
 * @file servo.cpp
 * @brief Servo control using TIM3 PWM channel 2.
 * @author Hayden Mai
 * @date May-13-2026
 */

#include "hal/servo.h"

namespace hal {

    bool Servo::init(TIM_HandleTypeDef *timer) { return pwm_.init(timer); }


    void Servo::set_pulse_us(uint16_t pulse_us) { pwm_.setPulseUs(pulse_us); }

} // namespace hal