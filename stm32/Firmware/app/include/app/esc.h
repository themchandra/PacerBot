#pragma once

#include <cstdint>

namespace esc {
constexpr uint16_t MIN_US {1000};
constexpr uint16_t NEUTRAL_US {1500};
constexpr uint16_t MAX_US {2000};

void init();
void set_pulse_us(uint16_t pulse_us);
} // namespace esc

// deadzone stops at 1550