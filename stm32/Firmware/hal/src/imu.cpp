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

    bool IMU::init(LowPassFilter filter)
    {
        // Wake device: clear SLEEP bit in PWR_MGMT_1 (reset value = 0x40)
        if (!write_byte(PWR_MGMT_1, 0x00))
            return false;

        if (!set_low_pass_filter(filter)) {
            return false;
        }

        // Optional: set accel full-scale to ±2 g (already default, explicit for clarity)
        if (!write_byte(ACCEL_CONFIG, 0x00))
            return false;

        // Optional: set gyro full-scale to ±250 °/s (already default)
        if (!write_byte(GYRO_CONFIG, 0x00))
            return false;

        return true;
    }


    bool IMU::set_low_pass_filter(LowPassFilter filter)
    {
        return write_byte(CONFIG, static_cast<uint8_t>(filter) & 0x07U);
    }


    bool IMU::write_byte(uint8_t reg, uint8_t value)
    {
        return HAL_I2C_Mem_Write(hi2c_, addr_, reg, 1, &value, 1, HAL_MAX_DELAY)
            == HAL_OK;
    }


    bool IMU::read_bytes(uint8_t reg, uint8_t *buf, uint16_t len)
    {
        return HAL_I2C_Mem_Read(hi2c_, addr_, reg, 1, buf, len, HAL_MAX_DELAY) == HAL_OK;
    }


    bool IMU::calibrate(uint16_t sample_count, uint32_t sample_delay_ms)
    {
        if (sample_count == 0U) {
            return false;
        }

        std::array<int64_t, 3> accel_sum {0, 0, 0};
        std::array<int64_t, 3> gyro_sum {0, 0, 0};

        uint8_t buf[14];
        for (uint16_t sample = 0; sample < sample_count; ++sample) {
            if (!read_bytes(ACCEL_XOUT_H, buf, sizeof(buf))) {
                return false;
            }

            std::array<int16_t, 3> raw_accel {};
            std::array<int16_t, 3> raw_gyro {};
            parse_axes(buf, raw_accel);
            parse_axes(buf + 8, raw_gyro);

            for (int axis = 0; axis < 3; ++axis) {
                accel_sum[axis] += raw_accel[axis];
                gyro_sum[axis] += raw_gyro[axis];
            }

            if ((sample + 1U) < sample_count && sample_delay_ms > 0U) {
                osDelay(sample_delay_ms);
            }
        }

        for (int axis = 0; axis < 3; ++axis) {
            accel_bias_g_[axis]  = static_cast<float>(accel_sum[axis])
                                 / static_cast<float>(sample_count) / ACCEL_SENSITIVITY;
            gyro_bias_dps_[axis] = static_cast<float>(gyro_sum[axis])
                                 / static_cast<float>(sample_count) / GYRO_SENSITIVITY;
        }

        std::printf("IMU calibrated: accel_bias=[%.4f, %.4f, %.4f] g, gyro_bias=[%.4f, "
                    "%.4f, %.4f] dps\n",
                    accel_bias_g_[0], accel_bias_g_[1], accel_bias_g_[2],
                    gyro_bias_dps_[0], gyro_bias_dps_[1], gyro_bias_dps_[2]);

        return true;
    }


    void IMU::parse_axes(const uint8_t *raw, std::array<int16_t, 3> &out)
    {
        out[0] = static_cast<int16_t>((raw[0] << 8) | raw[1]);
        out[1] = static_cast<int16_t>((raw[2] << 8) | raw[3]);
        out[2] = static_cast<int16_t>((raw[4] << 8) | raw[5]);
    }


    void IMU::apply_bias(std::array<float, 3> &values,
                         const std::array<float, 3> &bias) const
    {
        for (int axis = 0; axis < 3; ++axis) {
            values[axis] -= bias[axis];
        }
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
            apply_bias(accel, accel_bias_g_);
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
            apply_bias(gyro, gyro_bias_dps_);
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
            data.accel_g[i]  = raw_a[i] / ACCEL_SENSITIVITY - accel_bias_g_[i];
            data.gyro_dps[i] = raw_g[i] / GYRO_SENSITIVITY - gyro_bias_dps_[i];
        }
        return true;
    }

} // namespace hal