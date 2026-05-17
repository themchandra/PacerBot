/**
 * @file imu_task.cpp
 * @brief IMU sampling task for control loop consumption.
 * @author Hayden Mai
 * @date May-15-2026
 */

#include "app/imu_task.h"

#include <cmath>
#include <cstdio>

namespace app {

    namespace {
        constexpr float ACCEL_STATIONARY_THRESHOLD_MPS2 {0.05f};
        constexpr float GYRO_STATIONARY_THRESHOLD_DPS {1.0f};
    } // namespace

    IMUTask::IMUTask(I2C_HandleTypeDef *i2c, int imu_address) : imu_(i2c, imu_address) {}


    void IMUTask::start()
    {
        if (task_handle_ != nullptr) {
            return;
        }

        if (!imu_.init(FILTER)) {
            std::printf("IMU init failed\n");
            return;
        }

        std::printf("IMU init ok\n");

        if (!imu_.calibrate()) {
            std::printf("IMU calibration failed\n");
            return;
        }

        isRunning_   = true;
        task_handle_ = osThreadNew(threadTrampoline, this, &task_att_);
        if (task_handle_ == nullptr) {
            isRunning_ = false;
        }
    }


    void IMUTask::stop()
    {
        isRunning_ = false;

        if (task_handle_ != nullptr) {
            osThreadTerminate(task_handle_);
            task_handle_ = nullptr;
        }

        updated_ = false;
        latest_  = {};
    }


    bool IMUTask::get_data(Data &data_out)
    {
        if (!updated_) {
            return false;
        }

        data_out = latest_;
        updated_ = false;
        return true;
    }


    void IMUTask::threadTrampoline(void *args)
    {
        IMUTask *pThis = static_cast<IMUTask *>(args);
        pThis->threadLoop();
    }


    void IMUTask::threadLoop()
    {
        int fail_count = 0;
        uint32_t last_tick_ms {HAL_GetTick()};
        while (isRunning_) {
            const uint32_t current_tick_ms {HAL_GetTick()};
            uint32_t elapsed_ms {current_tick_ms - last_tick_ms};
            last_tick_ms = current_tick_ms;

            if (elapsed_ms == 0U) {
                elapsed_ms = 1U;
            }

            const float dt_sec = static_cast<float>(elapsed_ms) * 0.001f;

            // Attempt to read every cycle so velocity integration keeps advancing
            // even if the consumer has not yet fetched the previous sample.
            const bool ok = imu_.get_data(latest_.imu);
            if (ok) {
                const float accel_x_mps2 {latest_.imu.accel_g[0] * GRAVITY_MPS2};
                const float accel_y_mps2 {latest_.imu.accel_g[1] * GRAVITY_MPS2};

                const bool stationary {
                    (std::fabs(accel_x_mps2) < ACCEL_STATIONARY_THRESHOLD_MPS2)
                    && (std::fabs(accel_y_mps2) < ACCEL_STATIONARY_THRESHOLD_MPS2)
                    && (std::fabs(latest_.imu.gyro_dps[0])
                        < GYRO_STATIONARY_THRESHOLD_DPS)
                    && (std::fabs(latest_.imu.gyro_dps[1])
                        < GYRO_STATIONARY_THRESHOLD_DPS)
                    && (std::fabs(latest_.imu.gyro_dps[2])
                        < GYRO_STATIONARY_THRESHOLD_DPS)};

                if (stationary) {
                    latest_.velocity_x_mps = 0.0f;
                    latest_.velocity_y_mps = 0.0f;
                } else {
                    latest_.velocity_x_mps += accel_x_mps2 * dt_sec;
                    latest_.velocity_y_mps += accel_y_mps2 * dt_sec;
                }

                latest_.speed_mps
                    = std::sqrt((latest_.velocity_x_mps * latest_.velocity_x_mps)
                                + (latest_.velocity_y_mps * latest_.velocity_y_mps));
                updated_   = true;
                fail_count = 0;
            } else {
                ++fail_count;
                if ((fail_count % 50) == 0) {
                    std::printf("IMU read failed (%d attempts)\n", fail_count);
                }
            }

            osDelay(SAMPLE_PERIOD_MS);
        }
    }

} // namespace app
    