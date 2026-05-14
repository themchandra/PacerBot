/**
 * @file esc.h
 * @brief ESC Class, using PWM to control
 * @author Hayden Mai
 * @date May-13-2026
 */

#ifndef HAL_ESC_H_
#define HAL_ESC_H_

#include "hal/pwm.h"

#include <cstdint>

namespace hal {

    class ESC {
      public:
        // NOTE: Deadzone stops at 1550
        static constexpr uint16_t MIN_US {1000};
        static constexpr uint16_t NEUTRAL_US {1500};
        static constexpr uint16_t MAX_US {2000};

        bool init(TIM_HandleTypeDef *timer);
        void set_pulse_us(uint16_t pulse_us);

      private:
        static constexpr uint32_t TIM_CHANNEL {TIM_CHANNEL_1};

        PWM pwm_ {TIM_CHANNEL, MIN_US, MAX_US, NEUTRAL_US};
    };

} // namespace hal

#endif