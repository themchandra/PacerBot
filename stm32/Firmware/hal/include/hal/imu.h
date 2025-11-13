#ifndef IMU_H_
#define IMU_H_

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

// register addresses
constexpr int IMU_ADDRESS = 0x68;
constexpr int WHO_AM_I = 0x75;
constexpr int ACCEL_CONFIG = 0x1C;
constexpr int ACCEL_XOUT_H = 0x3B;
constexpr int ACCEL_YOUT_H = 0x3D;
constexpr int GYRO_XOUT_H = 0x43;

constexpr float ACCEL_SENSITIVITY = 16384.0; // LSB/g 
constexpr float GYRO_SENSITIVITY = 250.0; // deg/s

void scan_i2c(void);

void read_accel_data(void);

void read_gyro_data(void); 

 #endif