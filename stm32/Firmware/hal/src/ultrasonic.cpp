#include "hal/ultrasonic.h"

/*
static uint32_t IC_Val1 = 0;
static uint32_t IC_Val2 = 0;
static uint32_t Difference = 0;
static uint32_t Is_First_Captured = 0; // is it the first value captured?
static uint8_t Distance = 0;

void delay(uint16_t time, TIM_HandleTypeDef *htim) {
  __HAL_TIM_SET_COUNTER(htim, __COUNTER__);
  while (__HAL_TIM_GET_COUNTER(htim) < time);

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim){


  if (Is_First_Captured == 0) {
    IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1); // read the first value
    Is_First_Captured = 1;
    // change the polarity to falling edge
    __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
  }

  else if (Is_First_Captured == 1){ // if the first is already captured
    IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    __HAL_TIM_SET_COUNTER(htim, 0);

    if (IC_Val2 > IC_Val1){
      Difference = IC_Val2 - IC_Val1;
    }

    else if (IC_Val1 > IC_Val2){
      Difference = (0xffff - IC_Val1) + IC_Val2;
    }

    Distance = Difference * .034/2;
    Is_First_Captured = 0; // set it back to false

    // set polarity to rising edge
    __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);
  }

}

void HCSR04_Read(TIM_HandleTypeDef *htim) {
  HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

  __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC1);

}

*/