#pragma once

#include "main.h"

#include <cstdint>

namespace hal {

    class ESC {
      public:
        // NOTE: Deadzone stops at 1550
        static constexpr uint16_t MIN_US {1000};
        static constexpr uint16_t NEUTRAL_US {1550};
        static constexpr uint16_t MAX_US {2000};

        void init();
        void set_pulse_us(uint16_t pulse_us);

      private:
        static constexpr uint32_t ESC_CHANNEL {TIM_CHANNEL_1};

        TIM_HandleTypeDef *timer_ {nullptr};

        static uint16_t clampPulse(uint16_t pulseUs);
    };

} // namespace hal