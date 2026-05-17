/**
 * @file imu_task.h
 * @brief Task that reads IMU data for control loops.
 * @author Hayden Mai
 * @date May-15-2026
 */

#ifndef APP_IMU_TASK_H_
#define APP_IMU_TASK_H_

#include "hal/imu.h"

#include "cmsis_os.h"

#include <cstdint>

namespace app {
    /**
     * @class IMUTask
     * @brief Periodic IMU acquisition task for control-loop consumers.
     *
     * Samples IMU data at a fixed interval and exposes the latest unread sample
     * through a nonblocking polling API.
     */
    class IMUTask {
      public:
        struct Data {
            hal::IMU::Data imu {};
            float velocity_x_mps {0.0f};
            float velocity_y_mps {0.0f};
            float speed_mps {0.0f};
        };

        /**
         * @brief Construct IMU task with peripheral and device address.
         * @param i2c I2C handle used for sensor communication.
         * @param imu_address IMU I2C device address.
         */
        IMUTask(I2C_HandleTypeDef *i2c, int imu_address);

        /**
         * @brief Start the IMU sampling task.
         * @note If the task is already running, this call does nothing.
         */
        void start();

        /**
         * @brief Stop the IMU sampling task and clear buffered sample state.
         */
        void stop();

        /**
         * @brief Fetch the latest unread IMU sample in a nonblocking manner.
         * @param data_out Output reference populated with the latest sample when
         * available.
         * @return true if a new sample was returned, false if no unread sample exists.
         */
        bool get_data(Data &data_out);

      private:
        static constexpr uint32_t SAMPLE_PERIOD_MS {1};
        static constexpr hal::IMU::LowPassFilter FILTER {
            hal::IMU::LowPassFilter::BW_21Hz};
        static constexpr float GRAVITY_MPS2 {9.80665f};

        hal::IMU imu_;
        Data latest_ {};
        volatile bool updated_ {false};
        volatile bool isRunning_ {false};

        static constexpr uint32_t STACK_SIZE_BYTES {512};
        static constexpr osThreadAttr_t task_att_ {
            .name       = "IMUTask",
            .attr_bits  = 0,
            .cb_mem     = nullptr,
            .cb_size    = 0,
            .stack_mem  = nullptr,
            .stack_size = STACK_SIZE_BYTES,
            .priority   = osPriorityNormal,
            .tz_module  = 0,
            .reserved   = 0,
        };

        osThreadId_t task_handle_ {};

        /**
         * @brief Static RTOS entrypoint that forwards execution to the instance loop.
         * @param args Pointer to IMUTask instance.
         */
        static void threadTrampoline(void *args);

        /**
         * @brief Main periodic loop body for IMU sampling.
         */
        void threadLoop();
    };
} // namespace app

#endif
