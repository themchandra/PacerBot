#include "cmsis_os.h"
#include "hal/ultrasonic.h"
#include "main.h"
#include "stdio.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"
#include <cstdint>
#include <stdint.h>
#include <sys/_intsup.h>
#include <unistd.h>

namespace hal {
    Ultrasonic::Ultrasonic(TIM_HandleTypeDef *handle, uint32_t channel,
                           GPIO_TypeDef *trig_port, uint16_t trig_pin)
        : htim_(handle), channel_(channel), trig_port_(trig_port), trig_pin_(trig_pin)
    {
        instance = this;

        // Ensure the timer counter is running and the input-capture is started
        // so delay() (which relies on the timer counter) and capture IRQs work.
        HAL_TIM_Base_Start(htim_);
        HAL_TIM_IC_Start_IT(htim_, channel_);
        __HAL_TIM_SET_CAPTUREPOLARITY(htim_, channel_, TIM_INPUTCHANNELPOLARITY_RISING);
    }

    Ultrasonic *Ultrasonic::instance = nullptr;

    void Ultrasonic::delay(uint16_t time)
    {
        __HAL_TIM_SET_COUNTER(htim_, 0);
        while (__HAL_TIM_GET_COUNTER(htim_) < time)
            ;
    }

    void Ultrasonic::trigger()
    {
        HAL_GPIO_WritePin(trig_port_, trig_pin_, GPIO_PIN_SET);
        delay(10);
        HAL_GPIO_WritePin(trig_port_, trig_pin_, GPIO_PIN_RESET);

        __HAL_TIM_ENABLE_IT(htim_, TIM_IT_CC1);
    }

    float Ultrasonic::get_distance_cm() const { return distance_cm_; }


    void Ultrasonic::handle_capture_callback()
    {
        // first interrupt: ECHO rising edge
        // read that timestamp as the pulse start
        if (!first_captured_) {
            echo_rising_ticks_ = HAL_TIM_ReadCapturedValue(htim_, channel_);
            first_captured_    = true;

            // change the polarity to falling edge
            __HAL_TIM_SET_CAPTUREPOLARITY(htim_, channel_,
                                          TIM_INPUTCHANNELPOLARITY_FALLING);
        }

        // capture the falling edge of ECHO in ic_val2
        else {

            // Second interrupt: ECHO falling edge. Read timestamp as the pulse end
            echo_falling_ticks_ = HAL_TIM_ReadCapturedValue(htim_, channel_);

            // reset the timer for the next measurement
            __HAL_TIM_SET_COUNTER(htim_, 0);

            // normal case
            if (echo_falling_ticks_ > echo_rising_ticks_) {
                echo_pulse_ticks_ = echo_falling_ticks_ - echo_rising_ticks_;
            }

            // overflow case
            else if (echo_rising_ticks_ > echo_falling_ticks_) {
                uint32_t arr = __HAL_TIM_GET_AUTORELOAD(
                    htim_); // max value the timer counter can reach before wrapping to 0
                echo_pulse_ticks_ = (arr - echo_rising_ticks_ + 1u) + echo_falling_ticks_;
            }

            // divide by two since ECHO measures RTT
            distance_cm_    = echo_pulse_ticks_ * SPEED_OF_SOUND / 2;
            first_captured_ = false;

            // set polarity to rising edge
            __HAL_TIM_SET_CAPTUREPOLARITY(htim_, channel_,
                                          TIM_INPUTCHANNELPOLARITY_RISING);
            __HAL_TIM_DISABLE_IT(htim_, TIM_IT_CC1);
        }
    }
} // namespace hal