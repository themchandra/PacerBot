/**
 * @file pwm.h
 * @brief PWM output wrapper
 * @author Hayden Mai
 * @date May-13-2026
 */

#ifndef HAL_PWM_H_
#define HAL_PWM_H_

#include "main.h"
#include <cstdint>

namespace hal {

    class PWM {
      public:
        PWM(uint32_t channel, uint16_t min_us, uint16_t max_us, uint16_t neutral_us);

        bool init(TIM_HandleTypeDef *timer);
        void setPulseUs(uint16_t pulseUs);

      private:
        TIM_HandleTypeDef *timer_ {nullptr};
        uint32_t channel_ {0};
        uint16_t min_us_ {0};
        uint16_t max_us_ {0};
        uint16_t neutral_us_ {0};

        uint16_t clampPulse(uint16_t pulse_us) const;
    };

} // namespace hal

#endif