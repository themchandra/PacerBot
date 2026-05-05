#include "app/esc.h"

#include "main.h"

extern "C" {
extern TIM_HandleTypeDef htim3;
}

namespace esc {
void init()
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void set_pulse_us(uint16_t pulse_us)
{
    if (pulse_us < MIN_US) {
        pulse_us = MIN_US;
    }

    if (pulse_us > MAX_US) {
        pulse_us = MAX_US;
    }

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_us);
}
} // namespace esc
