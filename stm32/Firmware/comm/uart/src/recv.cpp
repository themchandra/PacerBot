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

    // Receiving buffers
    constexpr int RX_BUF_SIZE {512};
    std::array<uint8_t, RX_BUF_SIZE> rxBuf {};

    // Threading
    std::atomic_bool isThreadRunning_ {false};
    osSemaphoreId_t semThreadLoop_;

    // Task definition
    osThreadId_t taskHandle_;
    constexpr osThreadAttr_t task_att_ {
        .name       = "recvTask",
        .attr_bits  = 0,
        .cb_mem     = nullptr,
        .cb_size    = 0,
        .stack_mem  = nullptr,
        .stack_size = 128 * 4,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0,
    };


    void threadLoop(void *argument)
    {
        if (argument) {
        }


        while (isThreadRunning_) {
            // TODO: parse buffer
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            osDelay(1000);
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
        HAL_UARTEx_ReceiveToIdle_DMA(&huart_, rxBuf.data(), RX_BUF_SIZE);
        taskHandle_      = osThreadNew(threadLoop, NULL, &task_att_);
        isThreadRunning_ = true;
    }


    void stop()
    {
        assert(isInitialized_);
        isThreadRunning_ = false;

        // Wait until loop ends then "join" with other thread
        osSemaphoreAcquire(semThreadLoop_, osWaitForever);
    }

    bool isRunning()
    {
        assert(isInitialized_);
        return isThreadRunning_;
    }

} // namespace uart::recv