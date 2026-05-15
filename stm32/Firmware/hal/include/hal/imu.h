/**
 * @file imu.h
 * @brief IMU class
 * @author Michael Chandra
 * @date May-14-2026
 */

#ifndef HAL_IMU_H_
#define HAL_IMU_H_

#include "cmsis_os.h"
#include "main.h"

#include "stdio.h"
#include "stm32f411xe.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"
#include <array>
#include <stdint.h>
#include <sys/_intsup.h>
#include <unistd.h>

class IMU {
  public:
    struct Data {
        std::array<float, 3> accel_g {};
        std::array<float, 3> gyro_dps {};
    };

    void scan_i2c();
    void get_accel_raw(std::array<int, 3> &accel);
    void get_accel(std::array<float, 3> &accel);
    void get_gyro_raw(std::array<int, 3> &gyro);
    void get_gyro(std::array<float, 3> &gyro);
    Data get_data();
    IMU(I2C_HandleTypeDef *handle, int address);

  private:
    // register addresses
    static constexpr int WHO_AM_I     = 0x75;
    static constexpr int ACCEL_CONFIG = 0x1C;
    static constexpr int ACCEL_XOUT_H = 0x3B;
    static constexpr int ACCEL_YOUT_H = 0x3D;
    static constexpr int GYRO_XOUT_H  = 0x43;

    // scale factors
    static constexpr float ACCEL_SENSITIVITY = 16384.0; // LSB/g
    static constexpr float GYRO_SENSITIVITY  = 131.0;   // LSB/(deg/s)

    // i2c handler
    I2C_HandleTypeDef *hi2c_;
    int address_;
};
#endif