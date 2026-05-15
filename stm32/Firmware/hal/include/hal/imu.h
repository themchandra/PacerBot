/**
 * @file imu.h
 * @brief IMU (MPU-6050) HAL wrapper — STM32F4
 * @author Michael Chandra
 * @date May-15-2026
 */

#ifndef HAL_IMU_H_
#define HAL_IMU_H_

#include "cmsis_os.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include <array>
#include <cstdint>

namespace hal {

    class IMU {
      public:
        struct Data {
            std::array<float, 3> accel_g {};
            std::array<float, 3> gyro_dps {};
        };

        explicit IMU(I2C_HandleTypeDef *handle, uint8_t address);

        bool init();

        bool get_accel_raw(std::array<int16_t, 3> &accel);
        bool get_accel(std::array<float, 3> &accel);
        bool get_gyro_raw(std::array<int16_t, 3> &gyro);
        bool get_gyro(std::array<float, 3> &gyro);
        bool get_data(Data &data);

      private:
        // register map
        static constexpr uint8_t PWR_MGMT_1 {0x6B};
        static constexpr uint8_t WHO_AM_I {0x75};
        static constexpr uint8_t ACCEL_CONFIG {0x1C};
        static constexpr uint8_t GYRO_CONFIG {0x1B};
        // burst: 6 accel + 2 temp + 6 gyro = 14 bytes
        static constexpr uint8_t ACCEL_XOUT_H {0x3B};

        // scale factors (±2 g / ±250 °/s defaults)
        static constexpr float ACCEL_SENSITIVITY {16384.0f}; // LSB/g
        static constexpr float GYRO_SENSITIVITY {131.0f};    // LSB/(°/s)

        bool read_bytes(uint8_t reg, uint8_t *buf, uint16_t len);
        static void parse_axes(const uint8_t *raw, std::array<int16_t, 3> &out);

        I2C_HandleTypeDef *hi2c_;
        uint8_t addr_; // already shifted (addr << 1)
    };

} // namespace hal

#endif