/**
 * @file servo.h
 * @brief Servo control using TIM3 PWM channel 2.
 */

#ifndef HAL_SERVO_H_
#define HAL_SERVO_H_

#include "hal/pwm.h"

namespace hal {

    class Servo {
      public:
        static constexpr uint16_t MIN_US {1400}; // Right
        static constexpr uint16_t MAX_US {1700}; // Left
        static constexpr uint16_t CENTER_US {1550};

        bool init(TIM_HandleTypeDef *timer);
        void set_pulse_us(uint16_t pulse_us);

      private:
        static constexpr uint32_t TIM_CHANNEL {TIM_CHANNEL_2};

        Pwm pwm_ {TIM_CHANNEL, MIN_US, MAX_US, CENTER_US};
    };

} // namespace hal

#endif