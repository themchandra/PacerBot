/**
 * @file servo.h
 * @brief Servo control using TIM3 PWM channel 2.
 */

#ifndef HAL_SERVO_H_
#define HAL_SERVO_H_

#include "main.h"

#include <cstdint>

namespace hal {

    class Servo {
      public:
        bool init(TIM_HandleTypeDef *timer);
        void setPulseUs(uint16_t pulseUs);

      private:
        static constexpr uint32_t SERVO_CHANNEL {TIM_CHANNEL_2};
        static constexpr uint16_t SERVO_MIN_PULSE_US {1400};
        static constexpr uint16_t SERVO_MAX_PULSE_US {1700};
        static constexpr uint16_t SERVO_CENTER_PULSE_US {1550};

        TIM_HandleTypeDef *servoTimer_ {nullptr};

        static uint16_t clampPulse(uint16_t pulseUs);
    };

} // namespace hal

#endif