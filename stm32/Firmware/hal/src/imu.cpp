/**
 * @file imu.cpp
 * @brief IMU (MPU-6050) HAL wrapper — STM32F4
 * @author Michael Chandra
 * @date May-15-2026
 */

#include "hal/imu.h"
#include <cstdio>

namespace hal {

    IMU::IMU(I2C_HandleTypeDef *handle, uint8_t address)
        : hi2c_(handle), addr_(static_cast<uint8_t>(address << 1))
    {}

    bool IMU::init()
    {
        // Wake device: clear SLEEP bit in PWR_MGMT_1 (reset value = 0x40)
        uint8_t val = 0x00;
        if (HAL_I2C_Mem_Write(hi2c_, addr_, PWR_MGMT_1, 1, &val, 1, HAL_MAX_DELAY)
            != HAL_OK)
            return false;

        // Optional: set accel full-scale to ±2 g (already default, explicit for clarity)
        val = 0x00;
        if (HAL_I2C_Mem_Write(hi2c_, addr_, ACCEL_CONFIG, 1, &val, 1, HAL_MAX_DELAY)
            != HAL_OK)
            return false;

        // Optional: set gyro full-scale to ±250 °/s (already default)
        val = 0x00;
        if (HAL_I2C_Mem_Write(hi2c_, addr_, GYRO_CONFIG, 1, &val, 1, HAL_MAX_DELAY)
            != HAL_OK)
            return false;

        return true;
    }


    bool IMU::read_bytes(uint8_t reg, uint8_t *buf, uint16_t len)
    {
        return HAL_I2C_Mem_Read(hi2c_, addr_, reg, 1, buf, len, HAL_MAX_DELAY) == HAL_OK;
    }


    void IMU::parse_axes(const uint8_t *raw, std::array<int16_t, 3> &out)
    {
        out[0] = static_cast<int16_t>((raw[0] << 8) | raw[1]);
        out[1] = static_cast<int16_t>((raw[2] << 8) | raw[3]);
        out[2] = static_cast<int16_t>((raw[4] << 8) | raw[5]);
    }


    bool IMU::get_accel_raw(std::array<int16_t, 3> &accel)
    {
        uint8_t buf[6];
        bool ok = read_bytes(ACCEL_XOUT_H, buf, sizeof(buf));
        if (ok)
            parse_axes(buf, accel);
        return ok;
    }


    bool IMU::get_accel(std::array<float, 3> &accel)
    {
        std::array<int16_t, 3> raw;
        bool ok = get_accel_raw(raw);
        if (ok) {
            accel[0] = raw[0] / ACCEL_SENSITIVITY;
            accel[1] = raw[1] / ACCEL_SENSITIVITY;
            accel[2] = raw[2] / ACCEL_SENSITIVITY;
        }
        return ok;
    }


    bool IMU::get_gyro_raw(std::array<int16_t, 3> &gyro)
    {
        constexpr uint8_t GYRO_XOUT_H = ACCEL_XOUT_H + 8;
        uint8_t buf[6];
        bool ok = read_bytes(GYRO_XOUT_H, buf, sizeof(buf));
        if (ok)
            parse_axes(buf, gyro);
        return ok;
    }


    bool IMU::get_gyro(std::array<float, 3> &gyro)
    {
        std::array<int16_t, 3> raw;
        bool ok = get_gyro_raw(raw);
        if (ok) {
            gyro[0] = raw[0] / GYRO_SENSITIVITY;
            gyro[1] = raw[1] / GYRO_SENSITIVITY;
            gyro[2] = raw[2] / GYRO_SENSITIVITY;
        }
        return ok;
    }


    bool IMU::get_data(Data &data)
    {
        uint8_t buf[14];
        if (!read_bytes(ACCEL_XOUT_H, buf, sizeof(buf)))
            return false;

        std::array<int16_t, 3> raw_a, raw_g;
        parse_axes(buf, raw_a);     // bytes 0-5  → accel
        parse_axes(buf + 8, raw_g); // bytes 8-13 → gyro (bytes 6-7 = temperature)

        for (int i = 0; i < 3; ++i) {
            data.accel_g[i]  = raw_a[i] / ACCEL_SENSITIVITY;
            data.gyro_dps[i] = raw_g[i] / GYRO_SENSITIVITY;
        }
        return true;
    }

} // namespace hal