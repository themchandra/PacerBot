/*
#ifndef ULTRASONIC_H_
#define ULTRASONIC_H_

#include "main.h"
#include "cmsis_os.h"

#include "stdio.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"
#include <stdint.h>
#include <sys/_intsup.h>
#include <unistd.h>

const expr int TRIG_PIN = GPIO_PIN_10;
const expr int TRIG_PORT = GPIOB;

void delay(uint16_t time);

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

void HCSR04_Read(void);


 #endif
 */