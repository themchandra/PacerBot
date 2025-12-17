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


Ultrasonic::Ultrasonic(TIM_HandleTypeDef *handle, uint32_t channel,
                       GPIO_TypeDef *trig_port, uint16_t trig_pin)
    : htim_(handle), channel_(channel), trig_port_(trig_port), trig_pin_(trig_pin)
{
    instance = this;
}

Ultrasonic *Ultrasonic::instance = nullptr;

void Ultrasonic::delay(uint16_t time)
{
    __HAL_TIM_SET_COUNTER(htim_, __COUNTER__);
    while (__HAL_TIM_GET_COUNTER(htim_) < time)
        ;
}

void Ultrasonic::trigger()
{
    HAL_GPIO_WritePin(trig_port_, trig_pin_, GPIO_PIN_SET);
    // delay(10);
    HAL_GPIO_WritePin(trig_port_, trig_pin_, GPIO_PIN_RESET);

    __HAL_TIM_ENABLE_IT(htim_, TIM_IT_CC1);
}

float Ultrasonic::get_distance_cm() const { return distance_cm_; }


void Ultrasonic::handle_capture_callback()
{
    if (first_captured_ == 0) {
        ic_val1_ = HAL_TIM_ReadCapturedValue(htim_, channel_); // read the first value
        first_captured_ = 1;
        // change the polarity to falling edge
        __HAL_TIM_SET_CAPTUREPOLARITY(htim_, channel_, TIM_INPUTCHANNELPOLARITY_FALLING);
    }

    else if (first_captured_ == 1) { // if the first is already captured
        ic_val2_ = HAL_TIM_ReadCapturedValue(htim_, channel_);
        __HAL_TIM_SET_COUNTER(htim_, 0);

        // normal case
        if (ic_val2_ > ic_val1_) {
            diff_ = ic_val2_ - ic_val1_;
        }

        // overflow case
        else if (ic_val1_ > ic_val2_) {
            diff_ = (0xffff - ic_val1_) + ic_val2_;
        }

        // convert time to distance
        distance_cm_    = diff_ * SPEED_OF_SOUND / 2;
        first_captured_ = 0; // set it back to false

        // set polarity to rising edge
        __HAL_TIM_SET_CAPTUREPOLARITY(htim_, channel_, TIM_INPUTCHANNELPOLARITY_RISING);
        __HAL_TIM_DISABLE_IT(htim_, TIM_IT_CC1);
    }
}
