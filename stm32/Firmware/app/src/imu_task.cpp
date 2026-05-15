/**
 * @file imu_task.cpp
 * @brief IMU sampling task for control loop consumption.
 * @author Hayden Mai
 * @date May-14-2026
 */

#include "app/imu_task.h"

namespace app {

    IMUTask::IMUTask(I2C_HandleTypeDef *i2c, int imu_address) : imu_(i2c, imu_address) {}


    void IMUTask::start()
    {
        if (task_handle_ != nullptr) {
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


    bool IMUTask::get_data(IMU::Data &data_out)
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
        while (isRunning_) {
            if (!updated_) {
                latest_  = imu_.get_data();
                updated_ = true;
            }

            osDelay(SAMPLE_PERIOD_MS);
        }
    }

} // namespace app
