/**
 * @file recv.cpp
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-04-2025
 */

#include "cmsis_os.h"
#include "comm/uart/recv.h"

#include <atomic>
#include <cassert>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef huart_;

    // Threading
    std::atomic_bool isRunning_ {false};
    osSemaphoreId_t semThreadLoop_;

    // Task definition
    osThreadId_t taskHandle_;
    constexpr osThreadAttr_t task_att_ {
        .name       = "recvTask",
        .stack_size = 128 * 4,
        .priority   = (osPriority_t)osPriorityNormal,
    };


    void threadLoop(void *argument)
    {
        if (argument) {
        }


        while (isRunning_) {
            // TODO: parse buffer
        }

        osSemaphoreRelease(semThreadLoop_);
    }

} // namespace


namespace uart::recv {
    void init(UART_HandleTypeDef *huart)
    {
        assert(!isInitialized_);
        huart_         = *huart;
        semThreadLoop_ = osSemaphoreNew(1, 0, NULL);
        isInitialized_ = true;
    }


    void deinit()
    {
        assert(isInitialized_);
        isInitialized_ = false;
    }


    void start()
    {
        assert(isInitialized_);
        taskHandle_ = osThreadNew(threadLoop, NULL, &task_att_);
        isRunning_  = true;
    }


    void stop()
    {
        assert(isInitialized_);
        isRunning_ = false;

        // Wait until loop ends then "join" with other thread
        osSemaphoreAcquire(semThreadLoop_, osWaitForever);
    }

    bool isRunning()
    {
        assert(isInitialized_);
        return isRunning_;
    }

} // namespace uart::recv